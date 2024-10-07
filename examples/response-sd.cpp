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
#include <vsomeip/internal/logger.hpp>

#include "sample-ids.hpp"

class service_sample
{
public:
    service_sample(bool _use_static_routing) : app_(vsomeip::runtime::get()->create_application()),
                                               is_registered_(false),
                                               use_static_routing_(_use_static_routing),
                                               blocked_(false),
                                               running_(true),
                                               offer_thread_(std::bind(&service_sample::run, this))
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

        // app_->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
        //                                     std::bind(&service_sample::on_availability,
        //                                               this,
        //                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

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
        blocked_ = true;
        app_->clear_all_handler();
        stop_offer();
        condition_.notify_one();
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
                blocked_ = true;
                condition_.notify_one();
            }
        }
        else
        {
            is_registered_ = false;
        }
    }

    // void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
    // {
    //     if (_is_available)
    //     {
    //         // auto finished_time = std::chrono::high_resolution_clock::now();
    //         VSOMEIP_INFO << "service가 available 입니다";
    //         std::cout << "Service ["
    //                   << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
    //                   << "] is "
    //                   << (_is_available ? "available." : "NOT available.")
    //                   << std::endl;
    //     }
    // }

    void run()
    {
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!blocked_)
            condition_.wait(its_lock);
        offer();
        while (running_)
            ;
    }

private:
    std::shared_ptr<vsomeip::application>
        app_;
    bool is_registered_;
    bool use_static_routing_;

    std::mutex mutex_;
    std::condition_variable condition_;
    bool blocked_;

    bool running_;
    std::thread offer_thread_;
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