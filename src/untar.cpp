/***************************************************************************
* Copyright (c) 2021, Thorsten Beier                                       *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <string>
#include <archive.h>
#include <archive_entry.h>
#include <sstream>
#include <iostream>

namespace xeus
{
    static int  copy_data(struct archive *ar, struct archive *aw)
    {
        int r;
        const void *buff;
        size_t size;
        la_int64_t offset;

        for (;;) {
            r = archive_read_data_block(ar, &buff, &size, &offset);
            if (r == ARCHIVE_EOF){
                return (ARCHIVE_OK);
            }
            if (r < ARCHIVE_OK){
                return (r);
            }
            r = archive_write_data_block(aw, buff, size, offset);
            if (r < ARCHIVE_OK) {
                std::stringstream ss;
                ss  << "error: " << archive_error_string(aw) << std::endl;     
                throw std::runtime_error(ss.str());
            }
        }
    }

    void untar(std::string tar_path, std::string target_path){
        struct archive *a;
        struct archive *ext;
        struct archive_entry *entry;
        int flags;
        int r;

        a = archive_read_new();
        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, flags);
        archive_read_support_format_tar(a);
        archive_read_support_filter_gzip(a);
        r = archive_read_open_filename(a, tar_path.c_str(), 10240); // Note 1
        if (r != ARCHIVE_OK){
                std::stringstream ss;
                ss  << "xeus-lite untar error: !ARCHIVE_OK" << archive_error_string(a)
                    << " while extracting " << tar_path
                    << " to " << target_path << std::endl;
                throw std::runtime_error(ss.str());
            return;
        }

        for (;;) {

            r = archive_read_next_header(a, &entry);

            if (r == ARCHIVE_EOF){
                break;
            }
            if (r != ARCHIVE_OK){
                std::stringstream ss;
                ss  << "xeus-lite untar error: " << archive_error_string(a)
                    << " while extracting " << tar_path
                    << " to " << target_path << std::endl;
                throw std::runtime_error(ss.str());
            }

            const char* entry_path = archive_entry_pathname(entry);
            std::string entry_path_fs(entry_path);
            std::string target_path_fs(target_path);
            std::string full_path = target_path_fs + std::string("/") + entry_path_fs;

            archive_entry_set_pathname(entry, full_path.c_str());
            r = archive_write_header(ext, entry);
            if (r != ARCHIVE_OK){
                std::stringstream ss;
                ss  << "xeus-lite untar error: " << archive_error_string(ext)
                    << " while extracting " << tar_path
                    << " while writing " << full_path 
                    << " to " << target_path << std::endl;
                throw std::runtime_error(ss.str());
            }
            else if (archive_entry_size(entry) > 0) {
                r = copy_data(a, ext);
                if (r != ARCHIVE_OK || r == ARCHIVE_FATAL){
                    std::stringstream ss;
                    ss  << "xeus-lite untar error: " << archive_error_string(ext)
                        << " while extracting " << tar_path 
                        << " while writing " << full_path
                        << " to " << target_path << std::endl;
                    throw std::runtime_error(ss.str());
                }
            }
            r = archive_write_finish_entry(ext);
            if (r != ARCHIVE_OK)
            {
                std::stringstream ss;
                ss  << "xeus-lite untar error: " << archive_error_string(ext)
                    << " while extracting " << tar_path 
                    << " while writing " << full_path 
                    << " to " << target_path << std::endl;
                throw std::runtime_error(ss.str());
            }                
        }
        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);
    }
} // namespace xeus