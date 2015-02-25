/* funcdefs */

/**
 * GametypeDesc
 */
class GametypeDesc
{
public:
	/* object properties */
	uint spawnableItemsMask;
	uint respawnableItemsMask;
	uint dropableItemsMask;
	uint pickableItemsMask;
	bool isTeamBased;
	bool isRace;
	bool inverseScore;
	bool hasChallengersQueue;
	int maxPlayersPerTeam;
	int ammoRespawn;
	int armorRespawn;
	int weaponRespawn;
	int healthRespawn;
	int powerupRespawn;
	int megahealthRespawn;
	int ultrahealthRespawn;
	bool readyAnnouncementEnabled;
	bool scoreAnnouncementEnabled;
	bool countdownEnabled;
	bool mathAbortDisabled;
	bool shootingDisabled;
	bool infiniteAmmo;
	bool canForceModels;
	bool canShowMinimap;
	bool teamOnlyMinimap;
	int spawnpointRadius;
	bool customDeadBodyCam;
	bool mmCompatible;

	/* object behaviors */

	/* object methods */
	const String @ get_name() const;
	const String @ get_title() const;
	void set_title( String & );
	const String @ get_version() const;
	void set_version( String & );
	const String @ get_author() const;
	void set_author( String & );
	const String @ get_manifest() const;
	void setTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam );
	bool get_isInstagib() const;
	bool get_hasFallDamage() const;
	bool get_hasSelfDamage() const;
	bool get_isInvidualGameType() const;
};

