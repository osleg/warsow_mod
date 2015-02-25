/**
 * Dictionary
 */
class Dictionary
{
public:
	/* object properties */

	/* object behaviors */
	int _beh_13_();&s
	void _beh_14_();&s
	bool _beh_15_();&s
	void _beh_16_(int&in);&s
	void _beh_17_(int&in);&s
	Dictionary@ _beh_4_(int&in) { repeat { String ? } };&s

	/* object methods */
	Dictionary& opAssign(const Dictionary&in);
	void set(const String&in, ?&in);
	bool get(const String&in, ?&out) const;
	void set(const String&in, int64&in);
	bool get(const String&in, int64&out) const;
	void set(const String&in, double&in);
	bool get(const String&in, double&out) const;
	bool exists(const String&in) const;
	bool isEmpty() const;
	uint getSize() const;
	void delete(const String&in);
	void deleteAll();
	String[]@ getKeys() const;
	bool empty() const;
	uint size() const;
	void erase(const String&in);
	void clear();
};

