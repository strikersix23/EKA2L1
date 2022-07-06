/*
 * Copyright (c) 2022 EKA2L1 Team
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <services/bluetooth/protocols/asker_inet.h>

namespace eka2l1::epoc::bt {
    asker_inet::asker_inet()
        : bt_asker_(nullptr)
        , bt_asker_retry_timer_(nullptr) {

    }
    
    asker_inet::~asker_inet() {
        if (bt_asker_retry_timer_) {
            uv_timer_stop(bt_asker_retry_timer_);
            uv_close(reinterpret_cast<uv_handle_t*>(bt_asker_retry_timer_), [](uv_handle_t *hh) {
                delete hh;
            });
        }

        if (bt_asker_) {
            uv_udp_recv_stop(bt_asker_);
            uv_close(reinterpret_cast<uv_handle_t*>(bt_asker_), [](uv_handle_t *hh) {
                delete hh;
            });
        }
    }

    static constexpr std::uint32_t SEND_TIMEOUT_RETRY = 100;
    struct send_data_vars {
        uv_buf_t buf_;
        sockaddr_in6 addr_;
        asker_inet::response_callback response_cb_;
        char *response_ptr_ = nullptr;
        std::size_t response_size_;
        uv_udp_send_t *send_req;
        uv_udp_t *send_udp_handle_;
        uv_timer_t *timeout_timer_;
        uv_async_t *async_workload_;
        char *dynamic_buf_data_ = nullptr;
        int times_ = 0;
    };

    enum {
        BT_COMM_INET_ERROR_SEND_FAILED = -1,
        BT_COMM_INET_ERROR_RECV_FAILED = -2
    };

    static void free_send_data_vars_struct(send_data_vars *vars_ptr) {
        if (vars_ptr->response_ptr_) {
            free(vars_ptr->response_ptr_);
        }

        if (vars_ptr->send_req) {
            delete vars_ptr->send_req;
        }

        if (vars_ptr->dynamic_buf_data_) {
            delete vars_ptr->dynamic_buf_data_;
        }

        if (vars_ptr->async_workload_) {
            uv_close(reinterpret_cast<uv_handle_t*>(vars_ptr->async_workload_), [](uv_handle_t *hh) {
                uv_async_t *async_hh = reinterpret_cast<uv_async_t*>(hh);
                delete async_hh;
            });
        }

        delete vars_ptr;
    }

    static void keep_sending_data(uv_udp_t *handle, send_data_vars *vars_ptr) {
        uv_udp_send_t *send_req = new uv_udp_send_t;
        send_req->data = vars_ptr;
        handle->data = vars_ptr;
        vars_ptr->timeout_timer_->data = vars_ptr;
        vars_ptr->send_req = send_req;
        vars_ptr->send_udp_handle_ = handle;

        uv_async_t *async = new uv_async_t;
        vars_ptr->async_workload_ = async;
        async->data = vars_ptr;

        uv_async_init(uv_default_loop(), async, [](uv_async_t *async) {
            send_data_vars *vars_ptr = reinterpret_cast<send_data_vars*>(async->data);

            uv_udp_send(vars_ptr->send_req, vars_ptr->send_udp_handle_, &vars_ptr->buf_, 1, reinterpret_cast<sockaddr*>(&vars_ptr->addr_), [](uv_udp_send_t *send_info, int status) {
                send_data_vars *vars = reinterpret_cast<send_data_vars*>(send_info->data);
                vars->times_++;

                if (status >= 0) {
                    uv_udp_recv_start(send_info->handle, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
                        send_data_vars *vars = reinterpret_cast<send_data_vars*>(handle->data);
                        if (!vars->response_ptr_) {
                            vars->response_ptr_ = reinterpret_cast<char*>(malloc(suggested_size));
                            vars->response_size_ = suggested_size;
                        } else {
                            if (suggested_size > vars->response_size_) {
                                vars->response_ptr_ = reinterpret_cast<char*>(realloc(vars->response_ptr_, suggested_size));
                                vars->response_size_ = suggested_size;
                            }
                        }
                        buf->base = vars->response_ptr_;
                        buf->len = static_cast<std::uint32_t>(suggested_size);
                    }, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
                        send_data_vars *vars = reinterpret_cast<send_data_vars*>(handle->data);

                        uv_timer_stop(vars->timeout_timer_);
                        uv_udp_recv_stop(handle);

                        if (nread < 0) {
                            if (vars->times_ >= 3) {
                                vars->response_cb_(nullptr, BT_COMM_INET_ERROR_RECV_FAILED);
                                free_send_data_vars_struct(vars);
                            } else {
                                keep_sending_data(handle, vars);
                            }
                            return;
                        }

                        vars->response_cb_(buf->base, nread);
                        free_send_data_vars_struct(vars);

                        return;
                    });

                    uv_timer_start(vars->timeout_timer_, [](uv_timer_t *timer) {
                        send_data_vars *vars = reinterpret_cast<send_data_vars*>(timer->data);

                        uv_udp_recv_stop(vars->send_req->handle);
                        if (vars->times_ >= 3) {
                            vars->response_cb_(nullptr, BT_COMM_INET_ERROR_RECV_FAILED);
                            free_send_data_vars_struct(vars);
                        } else {
                            keep_sending_data(vars->send_req->handle, vars);
                        }
                    }, SEND_TIMEOUT_RETRY, 0);
                } else {
                    if (vars->times_ >= 3) {
                        vars->response_cb_(nullptr, BT_COMM_INET_ERROR_SEND_FAILED);
                        free_send_data_vars_struct(vars);

                        return;
                    } else {
                        keep_sending_data(vars->send_req->handle, vars);
                    }
                }
            });
        });

        uv_async_send(async);
    }

    void asker_inet::send_request_with_retries(const internet::sinet6_address &addr, char *request, const std::size_t request_size,
        response_callback response_cb) {
        if (!bt_asker_) {
            bt_asker_ = new uv_udp_t;

            sockaddr_in6 bind6;
            std::memset(&bind6, 0, sizeof(sockaddr_in6));
            bind6.sin6_family = AF_INET6;

            uv_udp_init(uv_default_loop(), bt_asker_);
            uv_udp_bind(bt_asker_, reinterpret_cast<const sockaddr*>(&bind6), UV_UDP_IPV6ONLY);
        }

        if (!bt_asker_retry_timer_) {
            bt_asker_retry_timer_ = new uv_timer_t;
            uv_timer_init(uv_default_loop(), bt_asker_retry_timer_);
        }

        sockaddr *addr_host;
        GUEST_TO_BSD_ADDR(addr, addr_host);

        send_data_vars *vars_ptr = new send_data_vars;

        vars_ptr->buf_ = uv_buf_init(request, static_cast<std::uint32_t>(request_size));
        vars_ptr->timeout_timer_ = bt_asker_retry_timer_;
        std::memcpy(&vars_ptr->addr_, addr_host, sizeof(sockaddr_in6));

        vars_ptr->response_cb_ = response_cb;
        keep_sending_data(bt_asker_, vars_ptr);
    }
}