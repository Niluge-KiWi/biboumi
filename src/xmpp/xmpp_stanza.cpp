#include <xmpp/xmpp_stanza.hpp>

#include <utils/encoding.hpp>
#include <utils/split.hpp>

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <cstring>

std::string xml_escape(const std::string& data)
{
  std::string res;
  res.reserve(data.size());
  for (size_t pos = 0; pos != data.size(); ++pos)
    {
      switch(data[pos])
        {
        case '&':
          res += "&amp;";
          break;
        case '<':
          res += "&lt;";
          break;
        case '>':
          res += "&gt;";
          break;
        case '\"':
          res += "&quot;";
          break;
        case '\'':
          res += "&apos;";
          break;
        default:
          res += data[pos];
          break;
        }
    }
  return res;
}

std::string sanitize(const std::string& data, const std::string& encoding)
{
  if (utils::is_valid_utf8(data.data()))
    return xml_escape(utils::remove_invalid_xml_chars(data));
  else
    return xml_escape(utils::remove_invalid_xml_chars(utils::convert_to_utf8(data, encoding.data())));
}

XmlNode::XmlNode(const std::string& name, XmlNode* parent):
  parent(parent)
{
  // split the namespace and the name
  auto n = name.rfind('\1');
  if (n == std::string::npos)
    this->name = name;
  else
    {
      this->name = name.substr(n+1);
      this->attributes["xmlns"] = name.substr(0, n);
    }
}

XmlNode::XmlNode(const std::string& name):
  XmlNode(name, nullptr)
{
}

XmlNode::XmlNode(const std::string& xmlns, const std::string& name, XmlNode* parent):
    name(name),
    parent(parent)
{
  this->attributes["xmlns"] = xmlns;
}

XmlNode::XmlNode(const std::string& xmlns, const std::string& name):
    XmlNode(xmlns, name, nullptr)
{
}

void XmlNode::delete_all_children()
{
  this->children.clear();
}

void XmlNode::set_attribute(const std::string& name, const std::string& value)
{
  this->attributes[name] = value;
}

void XmlNode::set_tail(const std::string& data)
{
  this->tail = data;
}

void XmlNode::add_to_tail(const std::string& data)
{
  this->tail += data;
}

void XmlNode::set_inner(const std::string& data)
{
  this->inner = data;
}

void XmlNode::add_to_inner(const std::string& data)
{
  this->inner += data;
}

std::string XmlNode::get_inner() const
{
  return this->inner;
}

std::string XmlNode::get_tail() const
{
  return this->tail;
}

const XmlNode* XmlNode::get_child(const std::string& name, const std::string& xmlns) const
{
  for (const auto& child: this->children)
    {
      if (child->name == name && child->get_tag("xmlns") == xmlns)
        return child.get();
    }
  return nullptr;
}

std::vector<const XmlNode*> XmlNode::get_children(const std::string& name, const std::string& xmlns) const
{
  std::vector<const XmlNode*> res;
  for (const auto& child: this->children)
    {
      if (child->name == name && child->get_tag("xmlns") == xmlns)
        res.push_back(child.get());
    }
  return res;
}

XmlNode* XmlNode::add_child(std::unique_ptr<XmlNode> child)
{
  child->parent = this;
  auto ret = child.get();
  this->children.push_back(std::move(child));
  return ret;
}

XmlNode* XmlNode::add_child(XmlNode&& child)
{
  auto new_node = std::make_unique<XmlNode>(std::move(child));
  return this->add_child(std::move(new_node));
}

XmlNode* XmlNode::add_child(const XmlNode& child)
{
  auto new_node = std::make_unique<XmlNode>(child);
  return this->add_child(std::move(new_node));
}

XmlNode* XmlNode::get_last_child() const
{
  return this->children.back().get();
}

XmlNode* XmlNode::get_parent() const
{
  return this->parent;
}

void XmlNode::set_name(const std::string& name)
{
  this->name = name;
}

void XmlNode::set_name(std::string&& name)
{
  this->name = std::move(name);
}

const std::string XmlNode::get_name() const
{
  return this->name;
}

std::string XmlNode::to_string() const
{
  std::ostringstream res;
  res << "<" << this->name;
  for (const auto& it: this->attributes)
    res << " " << it.first << "='" << sanitize(it.second) + "'";
  if (!this->has_children() && this->inner.empty())
    res << "/>";
  else
    {
      res << ">" + sanitize(this->inner);
      for (const auto& child: this->children)
        res << child->to_string();
      res << "</" << this->get_name() << ">";
    }
  res << sanitize(this->tail);
  return res.str();
}

bool XmlNode::has_children() const
{
  return !this->children.empty();
}

const std::string& XmlNode::get_tag(const std::string& name) const
{
  try
    {
      const auto& value = this->attributes.at(name);
      return value;
    }
  catch (const std::out_of_range& e)
    {
      static const std::string def{};
      return def;
    }
}

bool XmlNode::del_tag(const std::string& name)
{
  if (this->attributes.erase(name) != 0)
    return true;
  return false;
}

std::string& XmlNode::operator[](const std::string& name)
{
  return this->attributes[name];
}

std::ostream& operator<<(std::ostream& os, const XmlNode& node)
{
  return os << node.to_string();
}
