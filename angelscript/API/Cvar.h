/**
 * Cvar
 */
class Cvar
{
public:
	/* object properties */

	/* object behaviors */
	Cvar(const String&in, const String&in, const uint);&s
	Cvar(const Cvar&in);&s

	/* object methods */
	void reset();
	void set(const String&in);
	void set(float);
	void set(int);
	void set(double);
	bool get_modified() const;
	bool get_boolean() const;
	int get_integer() const;
	float get_value() const;
	const String@ get_name() const;
	const String@ get_string() const;
	const String@ get_defaultString() const;
	const String@ get_latchedString() const;
};

