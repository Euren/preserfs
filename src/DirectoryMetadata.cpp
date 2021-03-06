/*
    Copyright (C) 2011  Michael Schuerig <michael@schuerig.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DirectoryMetadata.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>
#include "Etc.h"
#include "exception.h"

using namespace std;
namespace fs = boost::filesystem;
namespace ser = boost::serialization;

const string DirectoryMetadata::metadataFilename = ".preserfs";

DirectoryMetadata::DirectoryMetadata(const EtcPtr etc)
    : etc_(etc)
{
}


DirectoryMetadata::Ptr
DirectoryMetadata::fromFilesystem(
    const string& directoryPath,
    NameShortener& shortener,
    const boost::shared_ptr<Etc> etc
) {
    fs::path path(directoryPath);
    Ptr dm(boost::make_shared<DirectoryMetadata>(etc));
    vector<DirectoryEntry>& entries(dm->entries_);
    
    for ( fs::directory_iterator it(path), end; it != end; ++it ) {
        const string entryPath( it->path().string() );

        struct stat st;
        if ( lstat(entryPath.c_str(), &st) < 0 ) {
            BOOST_THROW_EXCEPTION(
                MetadataError()
                << boost::errinfo_api_function("lstat")
                << boost::errinfo_file_name(entryPath)
                << boost::errinfo_errno(errno)
            );
        }

        const string entryName( it->path().filename().string() );

        DirectoryEntry entry = {
            longName    : entryName,
            shortName   : shortener(entryName),
            uid         : st.st_uid,
            gid         : st.st_gid,
            mode        : st.st_mode,
            mtime       : st.st_mtime,
        };

        entries.push_back(entry);
    }

    entries.swap(entries); // trim excessive capacity

    return dm;
}


DirectoryMetadata::Ptr
DirectoryMetadata::fromFilesystem(
    const string& directoryPath,
    NameShortener& shortener
) {
    EtcPtr etc(boost::make_shared<Etc>());
    return fromFilesystem(directoryPath, shortener, etc);
}


DirectoryMetadata::Ptr
DirectoryMetadata::fromMetadataFile(
    const string& directoryPath,
    const EtcPtr etc,
    const string& filename
) {
    fs::path path(directoryPath);
    path /= filename;
    Ptr dm(boost::make_shared<DirectoryMetadata>(etc));

    ifstream ifs(path.string().c_str());

    boost::archive::xml_iarchive ia(ifs);
    ia >> ser::make_nvp("entries", dm->entries_);

    return dm;
}


DirectoryMetadata::Ptr
DirectoryMetadata::fromMetadataFile(
    const string& directoryPath,
    const string& filename
) {
    EtcPtr etc(boost::make_shared<Etc>());
    return fromMetadataFile(directoryPath, etc, filename);
}

DirectoryMetadata::Ptr
DirectoryMetadata::fromMetadataFile(
    const string& directoryPath
) {
    return fromMetadataFile(directoryPath, metadataFilename);
}


void
DirectoryMetadata::write() const {
    fs::path path(metadataFilename);
    ofstream os(path.string().c_str());
    write(os);
}


void
DirectoryMetadata::write(ostream& os) const {
    boost::archive::xml_oarchive oa(os);
    oa << ser::make_nvp("entries", entries_);
}


namespace boost {
namespace serialization {

template<class Archive>
inline void serialize(
    Archive& ar,
    DirectoryEntry& e,
    const unsigned int file_version
) {
    boost::serialization::split_free(ar, e, file_version);
}


template<class Archive>
void save(
    Archive& ar,
    const DirectoryEntry& e,
    const unsigned int /*version*/
) {
    Etc etc; // TODO how to get hold of the instance in DirectoryMetadata from here?

    ar << ser::make_nvp("longname",  e.longName);
    ar << ser::make_nvp("shortname", e.shortName);
    ar << ser::make_nvp("owner",     etc.lookupUsername(e.uid));
    ar << ser::make_nvp("group",     etc.lookupGroupname(e.gid));
    ar << ser::make_nvp("mode",      e.mode);
    ar << ser::make_nvp("mtime",     e.mtime);
}


template<class Archive>
void load(
    Archive& ar,
    DirectoryEntry& e,
    const unsigned int /*version*/
) {
    Etc etc; // TODO how to get hold of the instance in DirectoryMetadata from here?
    
    ar >> ser::make_nvp("longname",  e.longName);
    ar >> ser::make_nvp("shortname", e.shortName);

    string owner;
    ar >> ser::make_nvp("owner",     owner);
    e.uid = etc.lookupUserId(owner);

    string group;
    ar >> ser::make_nvp("group",     group);
    e.gid = etc.lookupGroupId(group);

    ar >> ser::make_nvp("mode",      e.mode);
    ar >> ser::make_nvp("mtime",     e.mtime);
}

} // namespace serialization
} // namespace boost
