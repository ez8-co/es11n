#include "xpjson.hpp"
#include <list>
#include <deque>
#include <set>
#include <hash_set>
#include <hash_map>
#ifdef __XPJSON_SUPPORT_MOVE__
	#include <forward_list>
	#include <unordered_set>
#endif
using namespace std;
#ifndef _WIN32
	using namespace __gnu_cxx;
#else
	using namespace stdext;
#endif

#define _AS_ |

#define EZ_S11N_CUSTOM_CTOR_BASE EZ_S11N::custom_ctor_base
#define EZ_S11N_CUSTOM_CTOR(arglist) public:\
	template<class T>\
	static T s11n_custom_ctor() { return T arglist; }

#define EZ_S11N(members)\
	template<class char_t, class T> friend JSON::ValueT<char_t>& EZ_S11N::operator>>(JSON::ValueT<char_t>&, T&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& EZ_S11N::operator<<(JSON::ValueT<char_t>&, T&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& EZ_S11N::operator>>(T&, JSON::ValueT<char_t>&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& EZ_S11N::operator<<(T&, JSON::ValueT<char_t>&);\
	template<class char_t> EZ_S11N::Archive<char_t>& serialize(EZ_S11N::Archive<char_t>& json_s11n) { json_s11n.push(#members); return json_s11n | members;}\

namespace EZ_S11N
{
	using namespace JSON;

	struct custom_ctor_base{};
	template <typename Base, typename Derived>
	class is_base_of
	{
	    template <typename T>
	    static char helper(Derived, T);
	    static int helper(Base, int);
	    struct Conv {
	        operator Derived();
	        operator Base() const;
	    };
	public:
	    static const bool value = sizeof(helper(Conv(), 0)) == 1;
	};
	template<class T, bool custom = false>
	struct s11n_ctor { static T ctor() { return T(); } };
	template<class T>
	struct s11n_ctor<T, true> { static T ctor() { return T::template s11n_custom_ctor<T>(); } };

	template<class char_t>
	class Archive
	{
	public:
		string trim(const string& str)
		{
		    string::size_type pos = str.find_first_not_of(" \t\r\n*&");
		    if (pos == string::npos)
		        return str;
		    string::size_type pos2 = str.find_last_not_of(" \t\r\n");
		    if (pos2 != string::npos)
		        return str.substr(pos, pos2 - pos + 1);
		    return str.substr(pos);
		}
		string remove(const string& str, const string& src)
		{
		    string ret;
		    string::size_type pos_begin = 0, pos = 0;
		    while ((pos = str.find(src, pos_begin)) != string::npos) {
		        ret.append(str.data() + pos_begin, pos - pos_begin);
		        pos_begin = pos + 1;
		    }
		    if (pos_begin < str.length())
		        ret.append(str.begin() + pos_begin, str.end());
		    return ret;
		}
		Archive(ValueT<char_t>& v, bool s) : _from(0), _v(v), _s(s) {}
		void push(const string& s)
		{
			_arg_seq = remove(s, "\\\n");
		}
		string next()
		{
			int end = _arg_seq.find('|', _from);
			string s;
			if(end == string::npos) {
				s = _arg_seq.substr(_from);
				_from = 0;
				return beautify(s);
			}
			s = _arg_seq.substr(_from, end - _from);
			_from = end + 1;
			return beautify(s);
		}
		string beautify(const string& key)
		{
			int left = key.find('\"');
			int right = key.rfind('\"');
			return left != string::npos ? key.substr(left + 1, right - left - 1) : trim(key);
		}
		ValueT<char_t>& sub() {
			return _v[next()];
		}
		ValueT<char_t>& v() {
			return _v;
		}
		bool store() const {
			return _s;
		}
	private:
		int _from;
		string _arg_seq;
		ValueT<char_t>& _v;
		bool _s;
	};

	template<class char_t, size_t M>
	Archive<char_t>& operator _AS_(Archive<char_t>& s, const char (&v)[M]) { return s; }

	template<class char_t, class T>
	ValueT<char_t>& operator>>(ValueT<char_t>& in, T& v)
	{
		Archive<char_t> s(in, true);
		v.serialize(s);
		return in;
	}

	template<class char_t, class T>
	ValueT<char_t>& operator<<(ValueT<char_t>& out, T& v)
	{
		Archive<char_t> d(out, false);
		v.serialize(d);
		return out;
	}

	template<class char_t, class T>
	ValueT<char_t>& operator>>(T& v, ValueT<char_t>& out) { return out << v; }

	template<class char_t, class T>
	ValueT<char_t>& operator<<(T& v, ValueT<char_t>& in) { return in >> v; }

	template<class char_t, class T>
	Archive<char_t>& operator|(Archive<char_t>& s, T& v)
	{
		Archive<char_t> sub(s.sub(), s.store());
		v.serialize(sub);
		return s;
	}

#define S11N_HELPER(spec_type, default_value) \
	template<class char_t>\
	Archive<char_t>& operator|(Archive<char_t>& s, spec_type& v)\
	{\
		if(s.store()) {\
			v = s.v().template get<spec_type>(s.next(), default_value);\
		}\
		else {\
			s.sub() = v;\
		}\
		return s;\
	}

#define S11N_HELPER_ARITHMETIC(type) S11N_HELPER(type, 0)
	S11N_HELPER_ARITHMETIC(unsigned char)
	S11N_HELPER_ARITHMETIC(signed char)
#ifdef _NATIVE_WCHAR_T_DEFINED
	S11N_HELPER_ARITHMETIC(wchar_t)
#endif /* _NATIVE_WCHAR_T_DEFINED */
	S11N_HELPER_ARITHMETIC(unsigned short)
	S11N_HELPER_ARITHMETIC(signed short)
	S11N_HELPER_ARITHMETIC(unsigned int)
	S11N_HELPER_ARITHMETIC(signed int)
#if (defined (__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
	S11N_HELPER_ARITHMETIC(unsigned long)
	S11N_HELPER_ARITHMETIC(signed long)
#endif
	S11N_HELPER_ARITHMETIC(uint64_t)
	S11N_HELPER_ARITHMETIC(int64_t)
	S11N_HELPER_ARITHMETIC(float)
	S11N_HELPER_ARITHMETIC(double)
	S11N_HELPER_ARITHMETIC(long double)
	S11N_HELPER_ARITHMETIC(bool)

	S11N_HELPER(JSON_TSTRING(char_t), "")

	template<class char_t, class T>
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, T& v)
	{
		Archive<char_t> sub(out, s.store());
		v.serialize(sub);
	}

	template<class char_t, class T>
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, T& v)
	{
		Archive<char_t> sub(in, s.store());
		v.serialize(sub);
	}

#define S11N_VALUE(type) \
	template<class char_t> \
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, type& v) { out = v; }\
	template<class char_t>\
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, type& v) { v = in.template get<type>(); }

	S11N_VALUE(unsigned char)
	S11N_VALUE(signed char)
#ifdef _NATIVE_WCHAR_T_DEFINED
	S11N_VALUE(wchar_t)
#endif /* _NATIVE_WCHAR_T_DEFINED */
	S11N_VALUE(unsigned short)
	S11N_VALUE(signed short)
	S11N_VALUE(unsigned int)
	S11N_VALUE(signed int)
#if (defined (__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
	S11N_VALUE(unsigned long)
	S11N_VALUE(signed long)
#endif
	S11N_VALUE(uint64_t)
	S11N_VALUE(int64_t)
	S11N_VALUE(float)
	S11N_VALUE(double)
	S11N_VALUE(long double)
	S11N_VALUE(bool)
	S11N_VALUE(JSON_TSTRING(char_t))

#define S11N_VALUE_ARRAY_CONT(spec_type, insert_method) \
	template<class char_t, class T, class A>\
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, spec_type<T, A>& cont)\
	{\
		for(typename spec_type<T, A>::iterator it = cont.begin(); it != cont.end(); ++it) {\
			out.a().push_back(NIL);\
			to_s11n(s, out.a().back(), *it);\
		}\
	}\
	template<class char_t, class T, class A>\
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, spec_type<T, A>& cont)\
	{\
		if(in.type() == ARRAY) {\
			for(int i = 0; i < in.a().size(); ++i) {\
				T v = s11n_ctor<T, is_base_of<custom_ctor_base, T>::value>::ctor();\
				from_s11n(s, in.a()[i], v);\
				cont.insert_method(v);\
			}\
		}\
	}\
	template<class char_t, class T, class A>\
	Archive<char_t>& operator|(Archive<char_t>& s, spec_type<T, A>& cont)\
	{\
		if(s.store()) { from_s11n(s, s.sub(), cont); } else { to_s11n(s, s.sub(), cont); }\
		return s;\
	}

	S11N_VALUE_ARRAY_CONT(vector, push_back)
	S11N_VALUE_ARRAY_CONT(deque, push_back)
	S11N_VALUE_ARRAY_CONT(list, push_back)
	S11N_VALUE_ARRAY_CONT(set, insert)
	S11N_VALUE_ARRAY_CONT(hash_set, insert)
	S11N_VALUE_ARRAY_CONT(multiset, insert)
#ifdef __XPJSON_SUPPORT_MOVE__
	S11N_VALUE_ARRAY_CONT(forward_list, push_back)
	S11N_VALUE_ARRAY_CONT(unordered_set, insert)
	S11N_VALUE_ARRAY_CONT(unordered_multiset, insert)
#endif

	template<class char_t, class T, size_t M>
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, T (&arr)[M])
	{
		for (int i = 0; i < M; ++i) {
			out.a().push_back(NIL);
			to_s11n(s, out.a().back(), arr[i]);
		}
	}

	template<class char_t, class T, size_t M>
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, T (&arr)[M])
	{
		if(in.type() == ARRAY) {
			for(int i = 0; i < in.a().size(); ++i) {
				from_s11n(s, in.a()[i], arr[i]);
			}
		}
	}

	template<class char_t, class T, size_t M>
	Archive<char_t>& operator|(Archive<char_t>& s, T (&arr)[M])
	{
		if(s.store()) { from_s11n(s, s.sub(), arr); } else { to_s11n(s, s.sub(), arr); }
		return s;
	}

	template<class char_t, class T>
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, T*& ptr)
	{
		if(!ptr) return; to_s11n(s, out, *ptr);
	}

	template<class char_t, class T>
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, T*& ptr)
	{
		if(!ptr) return; from_s11n(s, in, *ptr);
	}

	template<class char_t, class T>
	Archive<char_t>& operator|(Archive<char_t>& s, T*& ptr)
	{
		if(!ptr) return s;
		if(s.store()) { from_s11n(s, s.sub(), *ptr); } else { to_s11n(s, s.sub(), *ptr); }
		return s;
	}

#define S11N_VALUE_KV_CONT(spec_type) \
	template<class char_t, class K, class V>\
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, spec_type<K, V> &cont)\
	{\
		for(typename spec_type<K, V>::iterator it = cont.begin(); it != cont.end(); ++it) {\
			ValueT<char_t> k_json;\
			to_s11n(s, k_json, const_cast<K&>(it->first));\
			JSON_TSTRING(char_t) key;\
			k_json.write(key);\
			to_s11n(s, out[key], it->second);\
		}\
	}\
	template<class char_t, class K, class V>\
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, spec_type<K, V> &cont)\
	{\
		if(in.type() == OBJECT) {\
			for(typename ObjectT<char_t>::iterator it = in.o().begin(); it != in.o().end(); ++it) {\
				ValueT<char_t> k_json;\
				k_json.read(it->first.c_str(), it->first.length());\
				K k = s11n_ctor<K, is_base_of<custom_ctor_base, K>::value>::ctor();\
				from_s11n(s, k_json, k);\
				V v = s11n_ctor<V, is_base_of<custom_ctor_base, V>::value>::ctor();\
				from_s11n(s, it->second, v);\
				cont.insert(JSON_MOVE(make_pair(k, v)));\
			}\
		}\
	}\
	template<class char_t, class K, class V>\
	Archive<char_t>& operator|(Archive<char_t>& s, spec_type<K, V> &cont)\
	{\
		if(s.store()) { from_s11n(s, s.sub(), cont); } else { to_s11n(s, s.sub(), cont); }\
		return s;\
	}

	S11N_VALUE_KV_CONT(map)
	S11N_VALUE_KV_CONT(multimap)
	S11N_VALUE_KV_CONT(hash_map)
#ifdef __XPJSON_SUPPORT_MOVE__
	S11N_VALUE_KV_CONT(unordered_map)
	S11N_VALUE_KV_CONT(unordered_multimap)
#endif
}