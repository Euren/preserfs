
#ifndef UTIL_H_INCLUDED_
#define UTIL_H_INCLUDED_

#include <cerrno>
#include <cstring>
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace util
{

void inline
throw_errno(const std::string& msg, const boost::filesystem::path& path, int err = errno) {
    throw boost::filesystem::filesystem_error(
        msg, path,
        boost::system::error_code(err, boost::system::system_category())
    );
}

void inline
throw_errno(const std::string& msg, int err = errno) {
    throw boost::system::system_error(
        boost::system::error_code(err, boost::system::system_category()),
	msg
    );
}

template<typename R, typename A1, typename A2>
class CatchAll {
public:
    typedef R (*func_t)(A1, A2);

    CatchAll(func_t fn, A1 a1, A2 a2)
	: fn_(fn), a1_(a1), a2_(a2) {}

    int call() {
	try {
	    r_ = fn_(a1_, a2_);
	} catch(const boost::system::system_error& ex) {
	    const boost::system::error_code err = ex.code();
	    std::cerr << ex.what() << std::endl;
	    return err.value();
	}
	return 0;
    }

    R result() {
	return r_;
    }

private:
    func_t fn_;
    R r_;
    A1 a1_;
    A2 a2_;
};


template<typename T, typename R>
std::map<R, typename T::value_type>
index_by(const T& t, R T::value_type::*ptr_to_mem)
{
    std::map<R, typename T::value_type> idx;
    for ( typename T::const_iterator i = t.begin(), end = t.end();
          i != end;
          ++i ) {
        idx[((*i).*ptr_to_mem)] = *i;
    }
    return  idx;
}

} // namespace util

#endif // UTIL_H_INCLUDED_
