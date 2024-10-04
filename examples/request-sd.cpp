// Copyright (C) 2014-2023 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <csignal>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vsomeip/vsomeip.hpp>
#include "sample-ids.hpp"
#include <vsomeip/internal/logger.hpp>

class service_discovery_client
{
public:
    service_discovery_client()
        : app_(vsomeip::runtime::get()->create_application()),
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

        app_->register_state_handler(
            std::bind(
                &service_discovery_client::on_state,
                this,
                std::placeholders::_1));

        app_->register_availability_handler(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
                                            std::bind(&service_discovery_client::on_availability,
                                                      this,
                                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        return true;
    }

    void start()
    {
        is_available_ = false;
        app_->start();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void stop()
    {
        app_->clear_all_handler();
        app_->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
        app_->stop();
    }

    void on_state(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            start_time = std::chrono::high_resolution_clock::now();
            VSOMEIP_INFO << "matching is started at: "
                         << std::chrono::duration_cast<std::chrono::microseconds>(start_time.time_since_epoch()).count()
                         << " μs";
            app_->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
        }
    }

    void on_availability(vsomeip::service_t _service, vsomeip::instance_t _instance, bool _is_available)
    {
        if (_is_available)
        {
            finished_time = std::chrono::high_resolution_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finished_time - start_time);
            VSOMEIP_INFO << "matching is finished at: "
                         << std::chrono::duration_cast<std::chrono::microseconds>(finished_time.time_since_epoch()).count()
                         << " μs";
            std::cout << "매칭까지 처리 시간: " << elapsed_ms.count() << "ms" << std::endl;
            std::cout << "Service ["
                      << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                      << "] is available." << std::endl;
            is_available_ = true;
            this->stop();
        }
        else
        {
            std::cout << "Service ["
                      << std::setw(4) << std::setfill('0') << std::hex << _service << "." << _instance
                      << "] is NOT available." << std::endl;
        }
    }

private:
    std::shared_ptr<vsomeip::application> app_;
    bool is_available_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> finished_time;
};

service_discovery_client *its_sample_ptr(nullptr);
void handle_signal(int _signal)
{
    if (its_sample_ptr != nullptr &&
        (_signal == SIGINT || _signal == SIGTERM))
    {
        its_sample_ptr->stop();
    }
}

int main()
{
    service_discovery_client its_sample;
    its_sample_ptr = &its_sample;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    for (int i = 0; i < 10; ++i)
    {
        if (its_sample.init())
        {
            std::cout << "Service Discovery iteration " << (i + 1) << " start." << std::endl;
            its_sample.start();
        }
        else
        {
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}