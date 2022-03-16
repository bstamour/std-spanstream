#ifndef BST_SPANSTREAM_HPP_
#define BST_SPANSTREAM_HPP_

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
    basic_spanbuf() : basic_spanbuf(std::ios_base::in | std::ios_base::out) {}

    explicit basic_spanbuf(std::ios_base::openmode which)
	: basic_spanbuf(std::span<charT>(), which) {}

    explicit basic_spanbuf(std::span<charT> s,
	std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
	: basic_streambuf<charT, traits>()
	, mode_(which)
    {
	this->span(s);
    }

    basic_spanbuf(const basic_spanbuf&) = delete;

    basic_spanbuf(basic_spanbuf&& rhs)
	: basic_streambuf<charT, traits>(std::move(rhs))
	, mode_(std::move(rhs.mode_))
	, buf_(std::move(rhs.buf_))
    {
    }

    // Assignment and swap
    basic_spanbuf& operator=(const basic_spanbuf&) = delete;

    basic_spanbuf& operator=(basic_spanbuf&& rhs)
    {
       basic_spanbuf tmp(std::move(rhs));
       this->swap(tmp);
       return *this;
    }

    void swap(basic_spanbuf& rhs)
    {
	std::basic_streambuf<charT, traits>::swap(rhs);
	std::swap(mode_, rhs.mode_);
	std::swap(buf_, rhs.buf_);
    }

    // Member functions
    std::span<charT> span() const noexcept
    {
	if (mode_ & std::ios_base::out)
	    return std::span<charT>(this->pbase(), this->pptr());
	else
	    return buf_;
    }

    void span(std::span<charT> s) noexcept
    {
	buf_ = s;

	if (mode_ & std::ios_base::out) {
	    this->setp(buf_.data(), buf_.data() + buf_.size());
	    if (mode_ & std::ios_base::ate) {
		this->pbump(buf_.size());
	    }
	}
	else if (mode_ & std::ios_base::in) {
	    this->setg(buf_.data(), buf_.data(), buf_.data() + buf_.size());
	}
    }

    friend void swap(basic_spanbuf& x, basic_spanbuf& y)
    {
	x.swap(y);
    }

protected:
    // Overridden virtual functions
    basic_streambuf<charT, traits>* setbuf(charT* s, std::streamsize n) override
    {
	this->span(std::span<charT>(s, n));
	return this;
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir way,
	std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
    {
	// TODO
	return pos_type{};
    }

    pos_type seekpos(pos_type sp,
	std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
    {
	return seekoff(off_type(sp), std::ios_base::beg, which);
    }

private:
    std::ios_base::openmode mode_;
    std::span<charT> buf_;
};

}

#endif
