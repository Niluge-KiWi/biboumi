#pragma once


#include <map>
#include <string>
#include <vector>
#include <memory>

std::string xml_escape(const std::string& data);
std::string xml_unescape(const std::string& data);
std::string sanitize(const std::string& data, const std::string& encoding = "ISO-8859-1");

/**
 * Represent an XML node. It has
 * - A parent XML node (in the case of the first-level nodes, the parent is
     nullptr)
 * - zero, one or more children XML nodes
 * - A name
 * - A map of attributes
 * - inner data (text inside the node)
 * - tail data (text just after the node)
 */
class XmlNode
{
public:
  explicit XmlNode(const std::string& name, XmlNode* parent);
  explicit XmlNode(const std::string& name);
  explicit XmlNode(const std::string& xmlns, const std::string& name, XmlNode* parent);
  explicit XmlNode(const std::string& xmlns, const std::string& name);
  /**
   * The copy constructor does not copy the parent attribute. The children
   * nodes are all copied recursively.
   */
  XmlNode(const XmlNode& node):
    name(node.name),
    parent(nullptr),
    attributes(node.attributes),
    children{},
    inner(node.inner),
    tail(node.tail)
  {
    for (const auto& child: node.children)
      this->add_child(std::make_unique<XmlNode>(*child));
  }

  XmlNode(XmlNode&& node) = default;
  XmlNode& operator=(const XmlNode&) = delete;
  XmlNode& operator=(XmlNode&&) = delete;

  ~XmlNode() = default;

  void delete_all_children();
  void set_attribute(const std::string& name, const std::string& value);
  /**
   * Set the content of the tail, that is the text just after this node
   */
  void set_tail(const std::string& data);
  /**
   * Append the given data to the content of the tail. This exists because
   * the expat library may provide the complete text of an element in more
   * than one call
   */
  void add_to_tail(const std::string& data);
  /**
   * Set the content of the inner, that is the text inside this node.
   */
  void set_inner(const std::string& data);
  /**
   * Append the given data to the content of the inner. For the reason
   * described in add_to_tail comment.
   */
  void add_to_inner(const std::string& data);
  /**
   * Get the content of inner
   */
  std::string get_inner() const;
  /**
   * Get the content of the tail
   */
  std::string get_tail() const;
  /**
   * Get a pointer to the first child element with that name and that xml namespace
   */
  const XmlNode* get_child(const std::string& name, const std::string& xmlns) const;
  /**
   * Get a vector of all the children that have that name and that xml namespace.
   */
  std::vector<const XmlNode*> get_children(const std::string& name, const std::string& xmlns) const;
  /**
   * Add a node child to this node. Assign this node to the child’s parent.
   * Returns a pointer to the newly added child.
   */
  XmlNode* add_child(std::unique_ptr<XmlNode> child);
  XmlNode* add_child(XmlNode&& child);
  XmlNode* add_child(const XmlNode& child);
  /**
   * Returns the last of the children. If the node doesn't have any child,
   * the behaviour is undefined. The user should make sure this is the case
   * by calling has_children() for example.
   */
  XmlNode* get_last_child() const;
  XmlNode* get_parent() const;
  void set_name(const std::string& name);
  void set_name(std::string&& name);
  const std::string get_name() const;
  /**
   * Serialize the stanza into a string
   */
  std::string to_string() const;
  /**
   * Whether or not this node has at least one child (if not, this is a leaf
   * node)
   */
  bool has_children() const;
  /**
   * Gets the value for the given attribute, returns an empty string if the
   * node as no such attribute.
   */
  const std::string& get_tag(const std::string& name) const;
  /**
   * Remove the attribute of the node. Does nothing if that attribute is not
   * present. Returns true if the tag was removed, false if it was absent.
   */
  bool del_tag(const std::string& name);
  /**
   * Use this to set an attribute's value, like node["id"] = "12";
   */
  std::string& operator[](const std::string& name);

private:
  std::string name;
  XmlNode* parent;
  std::map<std::string, std::string> attributes;
  std::vector<std::unique_ptr<XmlNode>> children;
  std::string inner;
  std::string tail;
};

std::ostream& operator<<(std::ostream& os, const XmlNode& node);

/**
 * An XMPP stanza is just an XML node of level 2 in the XMPP document (the
 * level 1 ones are the <stream::stream/>, and the ones above 2 are just the
 * content of the stanzas)
 */
using Stanza = XmlNode;

class XmlSubNode: public XmlNode
{
public:
    XmlSubNode(XmlNode& parent_ref, const std::string& name):
            XmlNode(name),
            parent_to_add(parent_ref)
    {}
    XmlSubNode(XmlNode& parent_ref, const std::string& xmlns, const std::string& name):
        XmlNode(xmlns, name),
        parent_to_add(parent_ref)
    {}

    ~XmlSubNode()
    {
      this->parent_to_add.add_child(std::move(*this));
    }
private:
    XmlNode& parent_to_add;
};