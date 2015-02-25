/* funcdefs */

/**
 * Team
 */
class Team
{
public:
	/* object properties */
	Stats stats;
	const int numPlayers;
	const int ping;
	const bool hasCoach;

	/* object behaviors */

	/* object methods */
	Entity @ ent( int index );
	const String @ get_name() const;
	const String @ get_defaultName() const;
	void set_name( String &in );
	bool isLocked() const;
	bool lock() const;
	bool unlock() const;
	void clearInvites();
	int team() const;
};

