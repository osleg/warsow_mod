/* funcdefs */

/**
 * Bot
 */
class Bot
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	void clearGoalWeights();
	void resetGoalWeights();
	void setGoalWeight( int i, float weight );
	float getItemWeight( const Item @item ) const;
	float get_reactionTime() const;
	float get_offensiveness() const;
	float get_campiness() const;
	float get_firerate() const;
};

