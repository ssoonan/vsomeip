// Copyright (C) 2014-2023 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <vsomeip/vsomeip.hpp>
#include <vsomeip/enumeration_types.hpp>

#include "sample-ids.hpp"

const char *state_to_string(vsomeip_v3::state_type_e state)
{
    switch (state)
    {
    case vsomeip_v3::state_type_e::ST_REGISTERED:
        return "ST_REGISTERED";
    case vsomeip_v3::state_type_e::ST_DEREGISTERED:
        return "ST_DEREGISTERED";
    default:
        return "Unknown State";
    }
}

class client_sample
{
public:
    client_sample(bool _use_tcp, bool _be_quiet)
        : app_(vsomeip::runtime::get()->create_application()),
          use_tcp_(_use_tcp),
          be_quiet_(_be_quiet),
          running_(true),
          is_available_(false)
    {
    }

    bool init()
    {
        if (!app_->init())
        {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }

        std::cout << "Client settings [protocol="
                  << (use_tcp_ ? "TCP" : "UDP")
                  << ":quiet="
                  << (be_quiet_ ? "true" : "false")
                  << "]"
                  << std::endl;

        app_->register_state_handler(
            std::bind(
                &client_sample::on_state,
                this,
                std::placeholders::_1));

        return true;
    }

    void start()
    {
        app_->start();
    }

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    /*
     * Handle signal to shutdown
     */
    void stop()
    {
        running_ = false;
        app_->clear_all_handler();
        app_->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
        app_->stop();
    }
#endif

    void on_state(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            start_time = std::chrono::steady_clock::now();
            app_->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
        }
    }

    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
    {
        if (_is_available)
        {
            end_time = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "매칭까지 처리 시간: " << elapsed_ms.count() << "ms" << std::endl;
        }

        std::cout << "Service ["
                  << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                  << "] is "
                  << (_is_available ? "available." : "NOT available.")
                  << std::endl;

        if (SAMPLE_SERVICE_ID == _service && SAMPLE_INSTANCE_ID == _instance)
        {
            if (is_available_ && !_is_available)
            {
                is_available_ = false;
            }
            else if (_is_available && !is_available_)
            {
                is_available_ = true;
            }
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool use_tcp_;
    bool be_quiet_;
    bool running_;
    bool is_available_;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> end_time;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
client_sample *its_sample_ptr(nullptr);
void handle_signal(int _signal)
{
    if (its_sample_ptr != nullptr &&
        (_signal == SIGINT || _signal == SIGTERM))
        its_sample_ptr->stop();
}
#endif

int main(int argc, char **argv)
{
    bool use_tcp = false;
    bool be_quiet = false;

    std::string tcp_enable("--tcp");
    std::string udp_enable("--udp");
    std::string quiet_enable("--quiet");

    int i = 1;
    while (i < argc)
    {
        if (tcp_enable == argv[i])
        {
            use_tcp = true;
        }
        else if (udp_enable == argv[i])
        {
            use_tcp = false;
        }
        else if (quiet_enable == argv[i])
        {
            be_quiet = true;
        }
        i++;
    }

    client_sample its_sample(use_tcp, be_quiet);
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif
    if (its_sample.init())
    {
        std::cout << "sample start\n";
        its_sample.start();
        return 0;
    }
    else
    {
        return 1;
    }
}
