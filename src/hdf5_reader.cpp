#include <bbp/sonata/hdf5_reader.h>

#include "hdf5_reader.hpp"
namespace {
class HDF5Lock
{
    bool _use_file_locking;
    bool _ignore_when_disabled;

  public:
    explicit HDF5Lock(bool use_file_locking, bool ignore_when_disabled)
        : _use_file_locking(use_file_locking)
        , _ignore_when_disabled(ignore_when_disabled) { }

    void apply(const hid_t list) const {
        herr_t err = H5Pset_file_locking(list, _use_file_locking, _ignore_when_disabled);
        if (err < 0) {
            HighFive::HDF5ErrMapper::ToException<HighFive::PropertyException>(
                "Unable to set non-locking when opening file.");
        }
    }
};
}  // namespace

namespace bbp {
namespace sonata {
HighFive::File openHDF5withoutLock(const std::string& path) {
    HighFive::FileAccessProps fapl = HighFive::FileAccessProps::Default();
    fapl.add(HDF5Lock(false, true));

    return HighFive::File(path, /*openFlags*/ HighFive::File::AccessMode::ReadOnly, fapl);
}


Hdf5Reader::Hdf5Reader()
    : impl(std::make_shared<
           Hdf5PluginDefault<Hdf5Reader::supported_1D_types, supported_2D_types>>()) { }

Hdf5Reader::Hdf5Reader(
    std::shared_ptr<Hdf5PluginInterface<supported_1D_types, supported_2D_types>> impl)
    : impl(std::move(impl)) { }

HighFive::File Hdf5Reader::openFile(const std::string& filename) const {
    return impl->openFile(filename);
}

}  // namespace sonata
}  // namespace bbp
