/**
 * DemoInfo
 */
class DemoInfo
{
public:
	/* object properties */

	/* object behaviors */
	~DemoInfo();&s
	DemoInfo();&s
	DemoInfo(const String&in);&s
	DemoInfo(const DemoInfo&in);&s

	/* object methods */
	DemoInfo& opAssign(const DemoInfo&in);
	const bool get_isPlaying() const;
	const bool get_isPaused() const;
	const uint get_time() const;
	void play() const;
	void stop() const;
	void pause() const;
	void jump(uint) const;
	const String@ get_name() const;
	void set_name(const String&in) const;
	String@ getMeta(const String&in) const;
};

