#include <utils/net.h>
#include <utils/flog.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <core.h>
#include <gui/style.h>
#include <config.h>
#include <gui/smgui.h>
#include <gui/tuner.h>
#include <gui/widgets/stepped_slider.h>
#include <utils/optionlist.h>

#include <string>

#include <zmq.h>

// VRT
#include <vrt/vrt_init.h>
#include <vrt/vrt_string.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>
#include <vrt/vrt_write.h>
#include <vrt/vrt_read.h>

#include "vrt-tools.h"

#define CONCAT(a, b) ((std::string(a) + b).c_str())

SDRPP_MOD_INFO{
    /* Name:            */ "vrtzmq_source",
    /* Description:     */ "VRT over ZMQ Source Module",
    /* Author:          */ "Thomas Telkamp",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ 1
};

ConfigManager config;

class VrtzmqSourceModule : public ModuleManager::Instance {
public:
    VrtzmqSourceModule(std::string name) {
        this->name = name;

        // samplerate = 1000000.0;

        handler.ctx = this;
        handler.selectHandler = menuSelected;
        handler.deselectHandler = menuDeselected;
        handler.menuHandler = menuHandler;
        handler.startHandler = start;
        handler.stopHandler = stop;
        handler.tuneHandler = tune;
        handler.stream = &stream;

        // Load config
        if (config.conf[name].contains("host")) {
            std::string hostStr = config.conf[name]["host"];
            strcpy(hostname, hostStr.c_str());
        }
        if (config.conf[name].contains("port")) {
            port = config.conf[name]["port"];
            port = std::clamp<int>(port, 1, 65535);
        }
        if (config.conf[name].contains("instance")) {
            instance = config.conf[name]["instance"];
            instance = std::clamp<int>(instance, 0, 65535);
        }
        if (config.conf[name].contains("use_port")) {
            use_port = config.conf[name]["use_port"];
            use_port = std::clamp<int>(use_port, 0, 1);
        }
        config.release();

        sigpath::sourceManager.registerSource("VRTZMQ", &handler);
    }

    ~VrtzmqSourceModule() {
        stop(this);
        sigpath::sourceManager.unregisterSource("VRTZMQ");
    }

    void postInit() {}

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

private:
    
    static void menuSelected(void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;
        // core::setInputSampleRate(_this->samplerate);
        flog::info("VrtzmqSourceModule '{0}': Menu Select!", _this->name);
    }

    static void menuDeselected(void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;
        flog::info("VrtzmqSourceModule '{0}': Menu Deselect!", _this->name);
    }

    static void start(void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;
        if (_this->running) { return; }
        
        try {
                // Open ZMQ 
                int hwm = 10000;
                uint16_t main_port;

                if (_this->use_port)
                    main_port = _this->port;
                else
                    main_port = DEFAULT_MAIN_PORT + MAX_CHANNELS*_this->instance;

                _this->context = zmq_ctx_new();
                _this->subscriber = zmq_socket(_this->context, ZMQ_SUB);
                int rc = zmq_setsockopt (_this->subscriber, ZMQ_RCVHWM, &hwm, sizeof hwm);
                std::string host(_this->hostname);
                std::string connect_string = "tcp://" + host + ":" + std::to_string(main_port);
                rc = zmq_connect(_this->subscriber, connect_string.c_str());
                assert(rc == 0);
                zmq_setsockopt(_this->subscriber, ZMQ_SUBSCRIBE, "", 0);
            }
        
        catch (const std::exception& e) {
            flog::error("Could not start VRTZMQ Source: {}", e.what());
            return;
        }

        _this->stopwork = false;

        // Start receive worker
        _this->workerThread = std::thread(&VrtzmqSourceModule::worker, _this);

        _this->running = true;
        flog::info("VrtzmqSourceModule '{0}': Start!", _this->name);
    }

    static void stop(void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;
        if (!_this->running) { return; }

        // Stop listen worker
        // TODO
        // ??

        _this->stopwork = true; // signal worker thread

        // Close connection
        // if (_this->sock) { _this->sock->close(); }
        zmq_close(_this->subscriber);
        zmq_ctx_destroy(_this->context);

        // Stop worker thread
        _this->stream.stopWriter();
        if (_this->workerThread.joinable()) { _this->workerThread.join(); }
        _this->stream.clearWriteStop();

        _this->running = false;
        flog::info("VrtzmqSourceModule '{0}': Stop!", _this->name);
    }

    static void tune(double freq, void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;
        if (_this->running) {
            // Nothing for now
        }
        _this->freq = freq;
        flog::info("VrtzmqSourceModule '{0}': Tune: {1}!", _this->name, freq);
    }

    static void menuHandler(void* ctx) {
        VrtzmqSourceModule* _this = (VrtzmqSourceModule*)ctx;

        if (_this->running) { SmGui::BeginDisabled(); }

        // Hostname and port field
        SmGui::LeftLabel("Host");
        if (SmGui::InputText(("##vrtzmq_source_host_" + _this->name).c_str(), _this->hostname, sizeof(_this->hostname))) {
            config.acquire();
            config.conf[_this->name]["host"] = _this->hostname;
            config.release(true);
        }
        // SmGui::SameLine();
        SmGui::LeftLabel("Instance");
        SmGui::FillWidth();
        if (SmGui::InputInt(("##vrtzmq_instance_" + _this->name).c_str(), &_this->instance, 0, 0)) {
            _this->instance = std::clamp<int>(_this->instance, 0, 65535);
            config.acquire();
            config.conf[_this->name]["instance"] = _this->instance;
            config.release(true);
        }

        SmGui::LeftLabel("Port");
        SmGui::FillWidth();
        if (SmGui::InputInt(("##vrtzmq_source_port_" + _this->name).c_str(), &_this->port, 0, 0)) {
            _this->port = std::clamp<int>(_this->port, 1, 65535);
            config.acquire();
            config.conf[_this->name]["port"] = _this->port;
            config.release(true);
        }
        // SmGui::SameLine();
        if (SmGui::Checkbox("Use Port", &_this->use_port)) {
            config.acquire();
            config.conf[_this->name]["use_port"] = _this->use_port;
            config.release(true);
        }

        if (_this->running) { SmGui::EndDisabled(); }
    }

    void worker() {

        uint32_t buffer[ZMQ_BUFFER_SIZE];
        float float_data[VRT_SAMPLES_PER_PACKET*2];

        context_type vrt_context;
        packet_type vrt_packet;

        init_context(&vrt_context);
        uint32_t channel = 0;
        vrt_packet.channel_filt = 1<<channel;

        bool start_rx = false;

        int64_t current_freq = 0;
        uint32_t current_sample_rate = 0;

        while (true) {
            // Read samples from ZMQ

            if (stopwork) // needed to close the thread
                break;
            
            size_t len = zmq_recv(subscriber, buffer, ZMQ_BUFFER_SIZE, 0);
            if (not vrt_process(buffer, sizeof(buffer), &vrt_context, &vrt_packet)) {
                printf("Not a Vita49 packet?\n");
                continue;
            }

            if (vrt_packet.context) {
                if (current_sample_rate != vrt_context.sample_rate) {
                    core::setInputSampleRate(vrt_context.sample_rate);
                    current_sample_rate = vrt_context.sample_rate;
                }
                if (current_freq != vrt_context.rf_freq) {
                    tuner::tune(tuner::TUNER_MODE_IQ_ONLY, "", vrt_context.rf_freq);
                    current_freq = vrt_context.rf_freq;
                }
                start_rx = true;
            }

            if (start_rx and vrt_packet.data) {
                for (uint32_t i = 0; i < vrt_packet.num_rx_samps; i++) {
                    int16_t re;
                    memcpy(&re, (char*)&buffer[vrt_packet.offset+i], 2);
                    int16_t img;
                    memcpy(&img, (char*)&buffer[vrt_packet.offset+i]+2, 2);
                    // convert to float32 and normalize
                    float_data[2*i] = (float)re / 32768.0;
                    float_data[2*i+1] = (float)img / 32768.0;
                }

                int count = vrt_packet.num_rx_samps;

                memcpy(stream.writeBuf, float_data, 2*vrt_packet.num_rx_samps*sizeof(float));
                // we can change this later to int16 and use volk for the conversion:
                // volk_16i_s32f_convert_32f((float*)stream.writeBuf, (int16_t*)dspbuffer, 32768.0f, count*2);
            
                // Send out converted samples
                if (!stream.swap(count)) { break; }
            }
        }
    }

    std::string name;
    bool enabled = true;
    dsp::stream<dsp::complex_t> stream;
    SourceManager::SourceHandler handler;
    bool running = false;
    bool stopwork = false;
    double freq;
    
    // int samplerate = 10000000;
    // int tempSamplerate = 10000000;
   
    char hostname[1024] = "localhost";
    int port = 50100;
    bool use_port = false;
    int instance = 0;

     // VRT ZMQ
    void *context;
    void *subscriber;

    std::thread workerThread;

};

MOD_EXPORT void _INIT_() {
    json def = json({});
    config.setPath(core::args["root"].s() + "/vrtzmq_source_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    return new VrtzmqSourceModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance) {
    delete (VrtzmqSourceModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}
