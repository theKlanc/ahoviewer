#ifndef _ZIP_H_
#define _ZIP_H_

#include "archive.h"

namespace AhoViewer
{
    class Zip : public Archive
    {
    public:
        Zip(const Glib::ustring &path, const Glib::ustring &exDir);
        virtual ~Zip() override = default;

        virtual bool extract(const Glib::ustring &file) const override;
        virtual bool has_valid_files(const FileType t) const override;
        virtual std::vector<std::string> get_entries(const FileType t) const override;

        static const int MagicSize = 4;
        static const char Magic[MagicSize];
    };
}

#endif /* _ZIP_H_ */
