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

#include "sample-ids.hpp"

class service_sample
{
public:
    service_sample(bool _use_static_routing) : app_(vsomeip::runtime::get()->create_application()),
                                               is_registered_(false),
                                               use_static_routing_(_use_static_routing)
    {
    }

    bool init()
    {
        std::lock_guard<std::mutex> its_lock(mutex_);

        if (!app_->init())
        {
            std::cerr << "Couldn't initialize application" << std::endl;
            return false;
        }
        app_->register_state_handler(
            std::bind(&service_sample::on_state, this,
                      std::placeholders::_1));
        app_->register_message_handler(
            SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_METHOD_ID,
            std::bind(&service_sample::on_find_message, this,
                      std::placeholders::_1));

        std::cout << "Static routing " << (use_static_routing_ ? "ON" : "OFF")
                  << std::endl;
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
        app_->stop();
    }
#endif

    void offer()
    {
        app_->offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    }

    void stop_offer()
    {
        app_->stop_offer_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    }

    void on_state(vsomeip::state_type_e _state)
    {
        std::cout << "Application " << app_->get_name() << " is "
                  << (_state == vsomeip::state_type_e::ST_REGISTERED ? "registered." : "deregistered.")
                  << std::endl;

        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            if (!is_registered_)
            {
                is_registered_ = true;
            }
        }
        else
        {
            is_registered_ = false;
        }
    }

    void on_find_message(const std::shared_ptr<vsomeip::message> &_request)
    {
        std::cout << "Received a find request with Client/Session ["
                  << std::setfill('0') << std::hex
                  << std::setw(4) << _request->get_client() << "/"
                  << std::setw(4) << _request->get_session() << "]"
                  << std::endl;

        // Once the find request is received, start offering the service.
        offer();
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool is_registered_;
    bool use_static_routing_;

    std::mutex mutex_;
    bool running_ = true;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
service_sample *its_sample_ptr(nullptr);
void handle_signal(int _signal)
{
    if (its_sample_ptr != nullptr &&
        (_signal == SIGINT || _signal == SIGTERM))
        its_sample_ptr->stop();
}
#endif

int main(int argc, char **argv)
{
    bool use_static_routing(false);

    std::string static_routing_enable("--static-routing");

    for (int i = 1; i < argc; i++)
    {
        if (static_routing_enable == argv[i])
        {
            use_static_routing = true;
        }
    }

    service_sample its_sample(use_static_routing);
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif
    if (its_sample.init())
    {
        its_sample.start();
        return 0;
    }
    else
    {
        return 1;
    }
}