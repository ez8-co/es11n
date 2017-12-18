#include "../ez_s11n.hpp"

#include <iostream>
using namespace std;

using namespace EZ_S11N;

class Nested
{
public:
	Nested(int64_t i = 0) : x(i) {
		b = new bool****;
		*b = new bool***;
		**b = new bool**;
		***b = new bool*;
		****b = new bool;
	}
	void print() const
	{
		cout << x << " " << d << " " <<  *****b << endl;
	}
	bool operator<(const Nested& other) const {
		return x < other.x;
	}
	bool ctor() {return true;}
	EZ_S11N(x _AS_ "1111"|d|b)

private:
	int64_t x;
	double d;
	bool***** b;
};

class Test
{
public:
	Test()
	{
		m[Nested(1)] = 10;
		m[Nested(2)] = 123123;
	}
	void print() const
	{
		cout << x << " " << dbl << " " <<  b << " " << ss << endl;
		for(vector<vector<string> >::const_iterator it = v.begin(); it != v.end(); ++it) {
			for(vector<string>::const_iterator i = it->begin(); i != it->end(); ++i) {
				cout << "string[][]= " << *i << endl;
			}
		}
		for(vector<Nested>::const_iterator it = o.begin(); it != o.end(); ++it) {
			cout << "   ";
			it->print();
		}
	}
	EZ_S11N(x _AS_ "sss"|dbl|b|o _AS_ "Nesteds"|ss _AS_ "test"|v|ia|m)

private:
	int64_t x;
	double dbl;
	bool b;
	string ss;
	vector<Nested> o;
	vector<vector<string> > v;
	int ia[2][2];
	map<Nested, int64_t> m;
};

int main(int argc, char** argv)
{
	JSON::Value v;
	v["sss"] = 100;
	v["dbl"] = 10.203;
	v["b"] = true;
	v["test"] = "I'm fine.";
	v["Nesteds"][0]["1111"] = 200;
	v["Nesteds"][0]["d"] = "157.8412";
	v["Nesteds"][0]["b"] = 0;
	v["v"][0][0] = 1;
	v["v"][0][1] = 1048576;
	v["v"][1][0] = 32423;
	v["v"][2][0] = "xxx";
	v["ia"][0][0] = 1;
	v["ia"][0][1] = 213123;
	v["ia"][1][0] = 2;
	v["ia"][1][1] = 14324324;

	Test t;
	t << v;
	t.print();

	JSON::Value v1;
	v1 << t;
	string s;
	v1.write(s);
	cout << s << endl;
	return 0;
}
