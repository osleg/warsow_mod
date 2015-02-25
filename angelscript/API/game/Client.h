/* funcdefs */

/**
 * Client
 */
class Client
{
public:
    /* object properties */
    Stats stats;
    const bool connecting;
    const bool multiview;
    const bool tv;
    int team;
    const int hand;
    const int fov;
    const int zoomFov;
    const bool isOperator;
    const uint queueTimeStamp;
    const int muted;
    float armor;
    uint gunbladeChargeTimeStamp;
    const bool chaseActive;
    int chaseTarget;
    bool chaseTeamonly;
    int chaseFollowMode;
    const bool coach;
    const int ping;
    const int16 weapon;
    const int16 pendingWeapon;
    bool takeStun;
    uint lastActivity;

    /* object behaviors */

    /* object methods */
    int get_playerNum() const;
    bool isReady() const;
    bool isBot() const;
    Bot @ getBot() const;
    int state() const;
    void respawn( bool ghost );
    void clearPlayerStateEvents();
    const String @ get_name() const;
    const String @ get_clanName() const;
    Entity @ getEnt() const;
    int inventoryCount( int tag ) const;
    void inventorySetCount( int tag, int count );
    void inventoryGiveItem( int tag, int count );
    void inventoryGiveItem( int tag );
    void inventoryClear();
    bool canSelectWeapon( int tag ) const;
    void selectWeapon( int tag );
    void addAward( const String &in );
    void addMetaAward( const String &in );
    void execGameCommand( const String &in );
    void setHUDStat( int stat, int value );
    int getHUDStat( int stat ) const;
    void set_pmoveFeatures( uint bitmask );
    void set_pmoveMaxSpeed( float speed );
    void set_pmoveJumpSpeed( float speed );
    void set_pmoveDashSpeed( float speed );
    uint get_pmoveFeatures() const;
    float get_pmoveMaxSpeed() const;
    float get_pmoveJumpSpeed() const;
    float get_pmoveDashSpeed() const;
    const String @ getUserInfoKey( const String &in ) const;
    void printMessage( const String &in );
    void chaseCam( const String @, bool teamOnly );
    void set_chaseActive( const bool active );
    bool get_chaseActive() const;
    void newRaceRun( int numSectors );
    void setRaceTime( int sector, uint time );
};

