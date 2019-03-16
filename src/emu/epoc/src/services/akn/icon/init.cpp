#include <epoc/services/akn/icon/icon.h>
#include <epoc/services/akn/icon/common.h>

#include <epoc/epoc.h>
#include <epoc/vfs.h>

#include <common/log.h>
#include <epoc/loader/rsc.h>

namespace eka2l1 {
    void akn_icon_server::init_server() {
        // Get the ROM device, and read the rsc
        eka2l1::io_system *io = sys->get_io_system();
        std::u16string path;

        // Find the ROM drive
        for (drive_number drv = drive_z; drv >= drive_a; drv = static_cast<drive_number>(drv - 1)) {
            auto result = io->get_drive_entry(drv);
            if (result && result->media_type == drive_media::rom) {
                path += drive_to_char16(drv);
                break;
            }
        }

        path += u":\\resource\\akniconsrv.rsc";
        symfile config_file = io->open_file(path, READ_MODE | BIN_MODE);

        if (!config_file) {
            LOG_ERROR("Can't find akniconsrv.rsc! Initialisation failed!");
            return;
        }

        // Read the file
        eka2l1::ro_file_stream config_file_stream(config_file);
        loader::rsc_file config_rsc(reinterpret_cast<common::ro_stream*>(&config_file_stream));

        // Read the initialisation data
        // The RSC data are layout as follow:
        // ---------------------------------------------------------------------------------------------       
        //    Index   | Size  |       Usage                   |       Note                             |
        //      1     |  1    |    Compression type           |  0 = No, 1 = RLE, 2 = Palette          |
        //      2     |  4    |    Preferred icon depth       |  0 = 64K color, 2 = 16K MU, else 16KM  |
        //      3     |  4    |    Preferred icon mask depth  |  0 = Gray2, else Gray256               |
        //      4     |  4    |    Photo depth                |  Same as icon depth                    |
        //      5     |  4    |    Video depth                |  Same as icon depth                    |
        //      6     |  4    |    Offscreen depth            |  Same as icon depth                    |
        //      7     |  4    |    Offscreen mask depth       |  Same as icon mask depth               |
        // ---------------------------------------------------------------------------------------------
        // More fields to come

        auto read_config_depth_to_display_mode = [&](const int idx) -> epoc::display_mode {
            auto data = config_rsc.read(idx);

            if (data.size() != 4) {
                LOG_ERROR("Try reading config depth, but size of resource is not equal to 4");
            } else {
                switch (*reinterpret_cast<std::uint32_t*>(&data[0])) {
                case 0: {
                    return epoc::display_mode::color64k;
                }

                case 2: {
                    return epoc::display_mode::color16mu;
                }

                default:
                    break;
                }
            }

            return epoc::display_mode::color16m;
        };

        auto read_config_mask_depth_to_display_mode = [&](const int idx) -> epoc::display_mode {
            auto data = config_rsc.read(idx);
            
            if (data.size() != 4) {
                LOG_ERROR("Try reading config mask depth, but size of resource is not equal to 4");
            } else {
                if (*reinterpret_cast<std::uint32_t*>(&data[0]) == 0) {
                    return epoc::display_mode::gray2;
                }
            }

            return epoc::display_mode::gray256;
        };

        init_data.compression = config_rsc.read(1)[0];
        init_data.icon_mode = read_config_depth_to_display_mode(2);
        init_data.icon_mask_mode = read_config_mask_depth_to_display_mode(3);
        init_data.photo_mode = read_config_depth_to_display_mode(4);
        init_data.video_mode = read_config_depth_to_display_mode(5);
        init_data.offscreen_mode = read_config_depth_to_display_mode(6);
        init_data.offscreen_mask_mode = read_config_mask_depth_to_display_mode(7);  

        // Done reading
        // Set the flags to inited.
        flags |= akn_icon_srv_flag_inited;
    }
};
