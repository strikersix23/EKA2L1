/*
 * Copyright (c) 2020 EKA2L1 Team.
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

#pragma once

#include <drivers/audio/backend/player_shared.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace eka2l1::drivers {
    struct player_ffmpeg : public player_shared {
    private:        
        AVCodecContext *codec_;
        AVFormatContext *format_context_;
        AVPacket packet_;

        const AVCodec *output_encoder_;
        std::uint64_t channel_layout_dest_;

        AVIOContext *custom_io_;
        std::uint8_t *custom_io_buffer_;
        std::uint64_t duration_us_;

    protected:
        bool is_ready_to_play() override;
        void deinit();
        bool open_ffmpeg_stream();

    public:
        explicit player_ffmpeg(audio_driver *driver);
        ~player_ffmpeg() override;

        bool make_backend_source() override;

        void reset_request() override;
        void get_more_data() override;

        bool open_url(const std::string &url) override;
        bool open_custom(common::rw_stream *stream) override;

        bool set_position_for_custom_format(const std::uint64_t pos_in_us) override;

        bool crop() override;
        bool record() override;

        bool set_dest_freq(const std::uint32_t freq) override;
        bool set_dest_channel_count(const std::uint32_t cn) override;
        bool set_dest_encoding(const std::uint32_t enc) override;

        std::uint64_t duration() const override {
            return duration_us_;
        }

        player_type get_player_type() const override {
            return player_type_ffmpeg;
        }
    };
}