/**
 * Element
 */
class Element
{
public:
	/* object properties */

	/* object behaviors */
	ElementDocument@ _beh_11_();&s
	ElementDataGrid@ _beh_11_();&s
	ElementDataGridRow@ _beh_11_();&s
	ElementForm@ _beh_11_();&s
	ElementFormControl@ _beh_11_();&s
	ElementFormControlDataSelect@ _beh_11_();&s
	ElementTabSet@ _beh_11_();&s
	ElementOptionsForm@ _beh_11_();&s

	/* object methods */
	bool setProp(const String&in, const String&in);
	String@ getProp(const String&in);
	void removeProp(const String&in);
	Element@ css(const String&in, const String&in);
	String@ css(const String&in);
	void setClass(const String&in, bool);
	bool hasClass(const String&in);
	void setClasses(const String&in);
	String@ getClasses();
	Element@ addClass(const String&in);
	Element@ removeClass(const String&in);
	Element@ toggleClass(const String&in);
	void toggleClass(const String&in, bool);
	void togglePseudo(const String&in, bool);
	bool hasPseudo(const String&in);
	Element@ setAttr(const String&in, const String&in);
	String@ getAttr(const String&in, const String&in);
	bool hasAttr(const String&in);
	void removeAttr(const String&in);
	int numAttr() const;
	String@ get_tagName() const;
	String@ get_id() const;
	void set_id(const String&in);
	Element@ getParent();
	Element@ getNextSibling();
	Element@ getPrevSibling();
	Element@ firstChild();
	Element@ lastChild();
	uint getNumChildren(bool = false);
	Element@ getChild(uint);
	String@ getInnerRML() const;
	void setInnerRML(const String&in);
	bool focus();
	void unfocus();
	void click();
	void addChild(Element@);
	void insertChild(Element@, Element@);
	void removeChild(Element@);
	bool hasChildren() const;
	Element@ clone();
	Element@ getElementById(const String&in);
	Element@[]@ getElementsByTagName(const String&in);
	Element@[]@ getElementsByClassName(const String&in);
	ElementDocument@ get_ownerDocument();
	void addEventListener(const String&inout, EventListenerCallback@);
	void removeEventListener(const String&in, EventListener@);
	float clientLeft();
	float clientTop();
	float clientHeight();
	float clientWidth();
	Element@ offsetParent();
	float offsetLeft();
	float offsetTop();
	float offsetHeight();
	float offsetWidth();
	float scrollLeft();
	void scrollLeft(float);
	float scrollTop();
	void scrollTop(float);
	float scrollHeight();
	float scrollWidth();
	float absLeft();
	float absTop();
};

