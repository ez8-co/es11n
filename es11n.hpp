#include "xpjson.hpp"
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
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

#define ES11N_CUSTOM_CTOR_BASE ES11N::custom_ctor_base
#define ES11N_CUSTOM_CTOR_EX(statement) public:\
	template<class T>\
	static T s11n_custom_ctor() { statement }
#define ES11N_CUSTOM_CTOR(...) ES11N_CUSTOM_CTOR_EX(return T(__VA_ARGS__);)

#define ES11N(members)\
	template<class char_t, class T> friend JSON::ValueT<char_t>& ES11N::operator>>(JSON::ValueT<char_t>&, T&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& ES11N::operator<<(JSON::ValueT<char_t>&, T&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& ES11N::operator>>(T&, JSON::ValueT<char_t>&);\
	template<class char_t, class T> friend JSON::ValueT<char_t>& ES11N::operator<<(T&, JSON::ValueT<char_t>&);\
	template<class char_t> ES11N::Archive<char_t>& serialize(ES11N::Archive<char_t>& json_s11n) { static ES11N::Schema s(#members); json_s11n.schema(s); return json_s11n | members;}\

namespace ES11N
{
	using namespace JSON;

	struct custom_ctor_base{};
	template <typename Base, typename Derived>
	struct is_base_of
	{
	    template <typename T>
	    static char helper(Derived, T);
	    static int helper(Base, int);
	    struct Conv {
	        operator Derived();
	        operator Base() const;
	    };
	    static const bool value = sizeof(helper(Conv(), 0)) == 1;
	};
	template<class T, bool custom = false>
	struct s11n_ctor { static T ctor() { return T(); } };
	template<class T>
	struct s11n_ctor<T, true> { static T ctor() { return T::template s11n_custom_ctor<T>(); } };

	struct Schema
	{
		Schema(const string& schema) {
		    string::size_type start = 0, end = 0;
		    do {
		        end = schema.find('|', start);
		        string::size_type quote = schema.find('\"', start = schema.find_first_not_of(" \t\r\n\\*&", start));
		        _schemas.push_back(quote >= end ? 
		        	schema.substr(start, schema.find_last_not_of(" \t\r\n\\", end - 1) - start + 1) : 
		        	schema.substr(quote + 1, schema.rfind('\"', end - 1) - quote - 1));
		    } while ((start = end + 1));
		}
		const string& index(int i) const { return _schemas[i]; }
	private:
		vector<string> _schemas;
	};

	template<class char_t>
	struct Archive
	{
		Archive(ValueT<char_t>& v, bool s) : _s(s), _index(0), _schema(0), _v(v)  {}
		void schema(const Schema& s) 	{ _schema = &s; }
		const string& next()			{ return _schema->index(_index++); }
		ValueT<char_t>& sub()			{ return _v[next()]; }
		ValueT<char_t>& v()				{ return _v; }
		bool store() const				{ return _s; }
	private:
		bool _s;
		int _index;
		const Schema* _schema;
		ValueT<char_t>& _v;
	};

	template<class char_t, size_t M>
	Archive<char_t>& operator _AS_(Archive<char_t>& s, const char (&)[M]) { return s; }

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

#define S11N_VALUE(type, default_value) \
	template<class char_t> \
	void to_s11n(Archive<char_t>& s, ValueT<char_t>& out, type& v) { out = v; }\
	template<class char_t>\
	void from_s11n(Archive<char_t>& s, ValueT<char_t>& in, type& v) { v = in.template get<type>(default_value); }

	S11N_VALUE(unsigned char, 0)
	S11N_VALUE(signed char, 0)
#ifdef _NATIVE_WCHAR_T_DEFINED
	S11N_VALUE(wchar_t, 0)
#endif /* _NATIVE_WCHAR_T_DEFINED */
	S11N_VALUE(unsigned short, 0)
	S11N_VALUE(signed short, 0)
	S11N_VALUE(unsigned int, 0)
	S11N_VALUE(signed int, 0)
#if (defined (__GNUC__) && !defined(__x86_64__)) || (defined(_WIN32) && !defined(_WIN64))
	S11N_VALUE(unsigned long, 0)
	S11N_VALUE(signed long, 0)
#endif
	S11N_VALUE(uint64_t, 0)
	S11N_VALUE(int64_t, 0)
	S11N_VALUE(float, 0)
	S11N_VALUE(double, 0)
	S11N_VALUE(long double, 0)
	S11N_VALUE(bool, 0)
	S11N_VALUE(JSON_TSTRING(char_t), "")

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