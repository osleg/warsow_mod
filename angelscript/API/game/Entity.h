/* funcdefs */
funcdef void entThink(Entity @ent);
funcdef void entTouch(Entity @ent, Entity @other, const Vec3 planeNormal, int surfFlags);
funcdef void entUse(Entity @ent, Entity @other, Entity @activator);
funcdef void entPain(Entity @ent, Entity @other, float kick, float damage);
funcdef void entDie(Entity @ent, Entity @inflicter, Entity @attacker);
funcdef void entStop(Entity @ent);

/**
 * Entity
 */
class Entity
{
public:
	/* object properties */
	Client @ client;
	Item @ item;
	Entity @ groundEntity;
	Entity @ owner;
	Entity @ enemy;
	Entity @ activator;
	int type;
	int modelindex;
	int modelindex2;
	int frame;
	int ownerNum;
	int counterNum;
	int skinNum;
	int colorRGBA;
	int weapon;
	bool teleported;
	uint effects;
	int sound;
	int team;
	int light;
	const bool inuse;
	uint svflags;
	int solid;
	int clipMask;
	int spawnFlags;
	int style;
	int moveType;
	uint nextThink;
	float health;
	int maxHealth;
	int viewHeight;
	int takeDamage;
	int damage;
	int projectileMaxDamage;
	int projectileMinDamage;
	int projectileMaxKnockback;
	int projectileMinKnockback;
	float projectileSplashRadius;
	int count;
	float wait;
	float delay;
	int waterLevel;
	float attenuation;
	int mass;
	uint timeStamp;
	entThink @ think;
	entTouch @ touch;
	entUse @ use;
	entPain @ pain;
	entDie @ die;
	entStop @ stop;
	int particlesSpeed;
	int particlesShaderIndex;
	int particlesSpread;
	int particlesSize;
	int particlesTime;
	int particlesFrequency;
	bool particlesSpherical;
	bool particlesBounce;
	bool particlesGravity;
	bool particlesExpandEffect;
	bool particlesShrinkEffect;

	/* object behaviors */

	/* object methods */
	Vec3 get_velocity() const;
	void set_velocity(const Vec3 &in);
	Vec3 get_avelocity() const;
	void set_avelocity(const Vec3 &in);
	Vec3 get_origin() const;
	void set_origin(const Vec3 &in);
	Vec3 get_origin2() const;
	void set_origin2(const Vec3 &in);
	Vec3 get_angles() const;
	void set_angles(const Vec3 &in);
	void getSize(Vec3 &out, Vec3 &out);
	void setSize(const Vec3 &in, const Vec3 &in);
	Vec3 get_movedir() const;
	void set_movedir();
	bool isBrushModel() const;
	void freeEntity();
	void linkEntity();
	void unlinkEntity();
	bool isGhosting() const;
	int get_entNum() const;
	int get_playerNum() const;
	const String @ get_model() const;
	const String @ get_model2() const;
	const String @ get_sounds() const;
	const String @ get_classname() const;
	const String @ get_targetname() const;
	const String @ get_target() const;
	const String @ get_map() const;
	void set_target( const String &in );
	void set_targetname( const String &in );
	void set_classname( const String &in );
	void set_map( const String &in );
	void ghost();
	void spawnqueueAdd();
	void teleportEffect( bool );
	void respawnEffect();
	void setupModel( const String &in );
	void setupModel( const String &in, const String &in );
	array<Entity @> @ findTargets() const;
	array<Entity @> @ findTargeting() const;
	void useTargets( const Entity @activator );
	Entity @ dropItem( int tag ) const;
	void sustainDamage( Entity @inflicter, Entity @attacker, const Vec3 &in dir, float damage, float knockback, float stun, int mod );
	void splashDamage( Entity @attacker, int radius, float damage, float knockback, float stun, int mod );
	void explosionEffect( int radius );
};

