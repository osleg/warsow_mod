/* funcdefs */

/**
 * Item
 */
class Item
{
public:
	/* object properties */
	const int tag;
	const uint type;
	const int flags;
	const int quantity;
	const int inventoryMax;
	const int ammoTag;
	const int weakAmmoTag;

	/* object behaviors */

	/* object methods */
	const String @ get_classname() const;
	const String @ get_name() const;
	const String @ get_shortName() const;
	const String @ get_model() const;
	const String @ get_model2() const;
	const String @ get_icon() const;
	const String @ get_simpleIcon() const;
	const String @ get_pickupSound() const;
	const String @ get_colorToken() const;
	bool isPickable() const;
	bool isUsable() const;
	bool isDropable() const;
};

