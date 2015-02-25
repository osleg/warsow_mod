/* funcdefs */

/**
 * Trace
 */
class Trace
{
public:
	/* object properties */
	const bool allSolid;
	const bool startSolid;
	const float fraction;
	const int surfFlags;
	const int contents;
	const int entNum;
	const float planeDist;
	const int16 planeType;
	const int16 planeSignBits;

	/* object behaviors */
	void f();
	void f(const Trace &in);

	/* object methods */
	bool doTrace( const Vec3 &in, const Vec3 &in, const Vec3 &in, const Vec3 &in, int ignore, int contentMask ) const;
	Vec3 get_endPos() const;
	Vec3 get_planeNormal() const;
};

