#ifndef BST_SPANSTREAM_HPP_
#define BST_SPANSTREAM_HPP_

#include <ranges>
#include <span>
#include <type_traits>

namespace bst {

//
// Synopsis
//

template <class charT, class traits = std::char_traits<charT>>
class basic_spanbuf;

using spanbuf = basic_spanbuf<char>;
using wspanbuf = basic_spanbuf<wchar_t>;

template <class charT, class traits = std::char_traits<charT>>
class basic_ispanstream;

using ispanstream = basic_ispanstream<char>;
using wispanstream = basic_ispanstream<wchar_t>;

template <class charT, class traits = std::char_traits<charT>>
class basic_ospanstream;

using ospanstream = basic_ospanstream<char>;
using wospanstream = basic_ospanstream<wchar_t>;

template <class charT, class traits = std::char_traits<charT>>
class basic_spanstream;

using spanstream = basic_spanstream<char>;
using wspanstream = basic_spanstream<wchar_t>;

//
// Class template basic_spanbuf
//

template <class charT, class traits = std::char_traits<charT>>
class basic_spanbuf : public std::basic_streambuf<charT, traits> {
public:
    using char_type = charT;
    using int_type = typename traits::int_type;
    using pos_type = typename traits::pos_type;
    using off_type = typename traits::off_type;
    using traits_type = traits;

    // Constructors
    basic_spanbuf()
        : basic_spanbuf(std::ios_base::in | std::ios_base::out) {}

    explicit basic_spanbuf(std::ios_base::openmode which)
        : basic_spanbuf(std::span<charT>(), which) {}

    explicit basic_spanbuf(std::span<charT> s,
                           std::ios_base::openmode which =
                               std::ios_base::in | std::ios_base::out)
        : basic_streambuf<charT, traits>(), mode_(which) {
        this->span(s);
    }

    basic_spanbuf(const basic_spanbuf&) = delete;

    basic_spanbuf(basic_spanbuf&& rhs)
        : basic_streambuf<charT, traits>(std::move(rhs)),
          mode_(std::move(rhs.mode_)), buf_(std::move(rhs.buf_)) {}

    // Assignment and swap
    basic_spanbuf& operator=(const basic_spanbuf&) = delete;

    basic_spanbuf& operator=(basic_spanbuf&& rhs) {
        basic_spanbuf tmp(std::move(rhs));
        this->swap(tmp);
        return *this;
    }

    void swap(basic_spanbuf& rhs) {
        std::basic_streambuf<charT, traits>::swap(rhs);
        std::swap(mode_, rhs.mode_);
        std::swap(buf_, rhs.buf_);
    }

    // Member functions
    std::span<charT> span() const noexcept {
        if (mode_ & std::ios_base::out)
            return std::span<charT>(this->pbase(), this->pptr());
        else
            return buf_;
    }

    void span(std::span<charT> s) noexcept {
        buf_ = s;

        if (mode_ & std::ios_base::out) {
            this->setp(buf_.data(), buf_.data() + buf_.size());
            if (mode_ & std::ios_base::ate) {
                this->pbump(buf_.size());
            }
        } else if (mode_ & std::ios_base::in) {
            this->setg(buf_.data(), buf_.data(), buf_.data() + buf_.size());
        }
    }

    friend void swap(basic_spanbuf& x, basic_spanbuf& y) { x.swap(y); }

protected:
    // Overridden virtual functions
    basic_streambuf<charT, traits>* setbuf(charT* s,
                                           std::streamsize n) override {
        this->span(std::span<charT>(s, n));
        return this;
    }

    pos_type
    seekoff(off_type off, std::ios_base::seekdir way,
            std::ios_base::openmode which = std::ios_base::in |
                                            std::ios_base::out) override {
        if ((which & std::ios_base::in) && (which & std::ios_base::out) &&
            way == std::ios_base::cur) {
            return pos_type(off_type(-1));
        }

        off_type baseoff;
        if (way == std::ios_base::beg)
            baseoff = 0;
        else if (way == std::ios_base::cur) {
            if (which & std::ios_base::in) {
                baseoff = this->gptr() - this->eback();
            } else if (which & std::ios_base::out) {
                baseoff = this->pptr() - this->pbase();
            }
        } else if (way == std::ios_base::end) {
            if ((mode_ & std::ios_base::out) &&
                !(mode_ & std::ios_base::in)) {
                baseoff = this->pptr() - this->pbase();
            } else {
                baseoff = buf_.size();
            }
        }

        if (baseoff + off < 0) {
            return pos_type(off_type(-1));
        }
        if (baseoff + off > buf_.size()) {
            return pos_type(off_type(-1));
        }

        off_type newoff = baseoff + off;

        // Input sequence.
        if (which & std::ios_base::in) {
            char_type* xnext = this->gptr();
            char_type* xbeg = this->eback();

            if (xnext == nullptr && newoff != 0)
                return pos_type(off_type(-1));

            xnext = xbegin + newoff;
            this->setg(xbeg, xnext, this->egptr());
        }

        // Output sequence.
        if (which & std::ios_base::out) {
            char_type* xnext = this->pptr();
            char_type* xbeg = this->pbase();

            if (xnext == nullptr && newoff != 0)
                return pos_type(off_type(-1));

            xnext = xbegin + newoff;
            this->setp(xbeg, this->epptr());
            this->pbump(newoff);
        }

        return pos_type(newoff);
    }

    pos_type
    seekpos(pos_type sp,
            std::ios_base::openmode which = std::ios_base::in |
                                            std::ios_base::out) override {
        return seekoff(off_type(sp), std::ios_base::beg, which);
    }

private:
    std::ios_base::openmode mode_;
    std::span<charT> buf_;
};

//
// class template basic_ispanstream
//
template <class charT, class traits = std::char_traits<charT>>
class basic_ispanstream : public std::basic_istream<charT, traits> {
public:
    using char_type = charT;
    using int_type = typename traits::int_type;
    using pos_type = typename traits::pos_type;
    using off_type = typename traits::off_type;
    using traits_type = traits;

    // constructors
    explicit basic_ispanstream(
        std::span<charT> s,
        std::ios_base::openmode which = std::ios_base::in)
        : std::basic_istream<charT, traits>(std::addressof(sb_)),
          sb_(s, which | std::ios_base::in) {}

    basic_ispanstream(const basic_ispanstream&) = delete;

    basic_ispanstream(basic_ispanstream&& rhs)
        : std::basic_istream<charT, traits>(std::move(rhs)),
          sb_(std::move(rhs.sb_)) {
        basic_istream<charT, traits>::set_rdbuf(std::addressof(sb_));
    }

    template <std::ranges::borrowed_range ROS>
    requires std::conjunction_v<
        std::negation<std::is_convertible_to<ROS, std::span<charT>>>,
        std::is_convertible_to<ROS, std::span<const charT>>>
    explicit basic_ispanstream(ROS&& s)
        : basic_ispanstream(make_temp_span(std::forward<ROS>(s))) {}

    // assignment and swap
    basic_ispanstream& operator=(const basic_ispanstream&) = delete;

    basic_ispanstream& operator=(basic_ispanstream&& rhs) {
        basic_ispanstream tmp(std::move(rhs));
        this->swap(tmp);
        return *this;
    }

    void swap(basic_ispanstream& rhs) {
        std::basic_istream<charT, traits>::swap(rhs);
        sb_.swap(rhs.sb_);
    }

    // member functions
    basic_spanbuf<charT, traits>* rdbuf() const noexcept {
        return const_cast<basic_spanbuf<charT, traits>*>(
            std::addressof(sb_));
    }

    std::span<const charT> span() const noexcept { return rdbuf()->span(); }
    void span(std::span<charT> s) noexcept { rdbuf()->span(s); }

    template <std::ranges::borrowed_range ROS>
    requires std::conjunction_v<
        std::negation<std::is_convertible_to<ROS, std::span<charT>>>,
        std::is_convertible_to<ROS, std::span<const charT>>>
    void span(ROS&& s) noexcept {
        this->span(make_temp_span(std::forward<ROS>(s)));
    }

    friend void swap(basic_ispanstream& x, basic_ispanstream& y) {
        x.swap(y);
    }

private:
    basic_spanbuf<charT, traits> sb_;

    template <class ROS>
    static std::span<charT> make_temp_span(ROS&& s) {
        std::span<const charT> sp(std::forward<ROS>(s));
        return std::span<charT>(const_cast<charT*>(sp.data()), sp.size());
    }
};

//
// class template basic_ospanstream
//
template <class charT, class traits = std::char_traits<charT>>
class basic_ospanstream : public std::basic_ostream<charT, traits> {
public:
    using char_type = charT;
    using int_type = typename traits::int_type;
    using pos_type = typename traits::pos_type;
    using off_type = typename traits::off_type;
    using traits_type = traits;

    // constructors
    explict
    basic_ospanstream(std::span<charT> s,
                      std::ios_base::openmode which = std::ios_base::out)
        : std::basic_ostream<charT, traits>(std::addressof(sb_)),
          sb_(s, which | std::ios_base::out) {}

    basic_ospanstream(const basic_ospanstream&) = delete;

    basic_ospanstream(basic_ospanstream&& rhs)
        : std::basic_ostream<charT, traits>(std::move(rhs)),
          sb_(std::move(rhs.sb_)) {
        std::basic_ostream<charT, traits>::set_rdbuf(std::addressof(sb_));
    }

    // assignment and swap
    basic_ospanstream& operator=(const basic_ospanstream&) = delete;

    basic_ospanstream& operator=(basic_ospanstream&& rhs) {
        basic_ospanstream tmp(std::move(rhs));
        this->swap(tmp);
        return *this;
    }

    void swap(basic_ospanstream& rhs) {
        std::basic_ostream<charT, traits>::swap(rhs);
        sb_.swap(rhs.sb_);
    }

    // member functions
    basic_spanbuf<charT, traits>* rdbuf() const noexcept {
        return const_cast<basic_spanbuf<charT, traits>*>(
            std::addressof(sb_));
    }

    std::span<charT> span() const noexcept { return rdbuf()->span(); }
    void span(std::span<charT> s) noexcept { rdbuf()->span(s); }

    friend void swap(basic_ospanstream& x, basic_ospanstream& y) {
        x.swap(y);
    }

private:
    basic_spanbuf<charT, traits> sb_;
};

//
// class template basic_spanstream
//
template <class charT, class traits = std::char_traits<charT>>
class basic_spanstream : public std::basic_iostream<charT, traits> {
public:
    using char_type = charT;
    using int_type = typename traits::int_type;
    using pos_type = typename traits::pos_type;
    using off_type = typename traits::off_type;
    using traits_type = traits;

    // constructors
    explicit basic_spanstream(std::span<charT> s,
                              std::ios_base::openmode which =
                                  std::ios_base::in | std::ios_base::out)
        : std::basic_iostream<charT, traits>(std::addressof(sb_)),
          sb_(s, which) {}

    basic_spanstream(const basic_spanstream&) = delete;

    basic_spanstream(basic_spanstream&& rhs)
        : std::basic_iostream<charT, traits>(std::move(rhs)),
          sb_(std::move(rhs.sb_)) {
        std::basic_iostream<charT, traits>::set_rdbuf(std::addressof(sb_));
    }

    // assignment and swap
    basic_spanstream& operator=(const basic_spanstream&) = delete;

    basic_spanstream& operator=(basic_spanstream&& rhs) {
        basic_spanstream tmp(std::move(rhs));
        this->swap(tmp);
        return *this;
    }

    void swap(basic_spanstream& rhs) {
        std::basic_iostream<charT, traits>::swap(rhs);
        sb_.swap(rhs.sb_);
    }

    // members
    basic_spanbuf<charT, traits>* rdbuf() const noexcept {
        return const_cast<basic_spanbuf<charT, traits>*>(
            std::addressof(sb_));
    }

    std::span<charT> span() const noexcept { return rdbuf()->span(); }
    void span(std::span<charT> s) noexcept { rdbuf()->span(s); }

    friend void swap(basic_spanstream& x, basic_spanstream& y) {
        x.swap(y);
    }

private:
    basic_spanbuf<charT, traits> sb_;
};

} // namespace bst

#endif
