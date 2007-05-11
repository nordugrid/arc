// MessageAttributes.h

#ifndef __ARC_MESSAGE_ATTRIBUTES__
#define __ARC_MESSAGE_ATTRIBUTES__

#include <map>
#include <string>

namespace Arc {

  //! A typefed of a multimap for storage of message attributes.
  /*! This typedef is used as a shorthand for a multimap that uses
    strings for keys as well as values. It is used within the
    MesssageAttributes class for internal storage of message
    attributes, but is not visible externally.
  */
  typedef std::multimap<std::string,std::string> AttrMap;


  //! A typedef of a const_iterator for AttrMap.
  /*! This typedef is used as a shorthand for a const_iterator for
    AttrMap. It is used extensively within the MessageAttributes class
    as well as the AttributesIterator class, but is not visible
    externally.
   */
  typedef AttrMap::const_iterator AttrConstIter;


  //! A typedef of an (non-const) iterator for AttrMap.
  /*! This typedef is used as a shorthand for a (non-const) iterator
    for AttrMap. It is used in one method within the MessageAttributes
    class, but is not visible externally.
   */
  typedef AttrMap::iterator AttrIter;


  //! An iterator class for accessing multiple values of an attribute.
  /*! This is an iterator class that is used when accessing multiple
    values of an attribute. The getAll() method of the
    MessageAttributes class returns an AttributeIterator object that
    can be used to access the values of the attribute.

    Typical usage is:
    \code
    Arc::MessageAttributes attributes;
    ...
    for (Arc::AttributeIterator iterator=attributes.getAll("Foo:Bar");
         iterator.hasMore(); ++iterator)
      std::cout << *iterator << std::endl;
    \endcode
   */
  class AttributeIterator {
  public:

    //! Default constructor.
    /*! The default constructor. Does nothing since all attributes are
      instances of well-behaving STL classes.
     */
    AttributeIterator();

    //! The dereference operator.
    /*! This operator is used to access the current value referred to
      by the iterator.
      \return A (constant reference to a) string representation of the
      current value.
     */
    const std::string& operator*() const;

    //! The arrow operator.
    /*! Used to call methods for value objects (strings) conveniently.
     */
    const std::string* operator->() const;

    //! The prefix advance operator.
    /*! Advances the iterator to the next value. Works intuitively.
      \return A const reference to this iterator.
     */
    const AttributeIterator& operator++();

    //! The postfix advance operator.
    /*! Advances the iterator to the next value. Works intuitively.
      \return An iterator referring to the value referred to by this
      iterator before the advance.
     */
    AttributeIterator operator++(int);

    //! Predicate method for iteration termination.
    /*! This method determines whether there are more values for the
      iterator to refer to.
      \return Returns true if there are more values, otherwise false.
     */
    bool hasMore() const;

  protected:

    //! Protected constructor used by the MessageAttributes class.
    /*! This constructor is used to create an iterator for iteration
      over all values of an attribute. It is not supposed to be
      visible externally, but is only used from within the getAll()
      method of MessageAttributes class.
      \param begin A const_iterator pointing to the first matching
      key-value pair in the internal multimap of the MessageAttributes
      class.
      \param end A const_iterator pointing to the first key-value pair
      in the internal multimap of the MessageAttributes class where
      the key is larger than the key searched for.
     */
    AttributeIterator(AttrConstIter begin, AttrConstIter end);

    //! A const_iterator pointing to the current key-value pair.
    /*! This iterator is the internal representation of the current
      value. It points to the corresponding key-value pair in the
      internal multimap of the MessageAttributes class.
    */
    AttrConstIter current_;

    //! A const_iterator pointing beyond the last key-value pair.
    /*! A const_iterator pointing to the first key-value pair in the
      internal multimap of the MessageAttributes class where the key
      is larger than the key searched for.
    */
    AttrConstIter end_;

    //! The MessageAttributes class is a friend.
    /*! The constructor that creates an AttributeIterator that is
      connected to the internal multimap of the MessageAttributes
      class should not be exposed to the outside, but it still needs
      to be accessible from the getAll() method of the
      MessageAttributes class. Therefore, that class is a friend.
     */
    friend class MessageAttributes;

  };


  //! A class for storage of attribute values.
  /*! This class is used to store attributes of messages. All
    attribute keys and their corresponding values are stored as
    strings. Any key or value that is not a string must thus be
    represented as a string during storage. Furthermore, an attribute
    is usually a key-value pair with a unique key, but there may also
    be multiple such pairs with equal keys.

    The key of an attribute is composed by the name of the Message
    Chain Component (MCC) which produce it and the name of the
    attribute itself with a colon (:) in between,
    i.e. MCC_Name:Attribute_Name. For example, the key of the
    "Content-Length" attribute of the HTTP MCC is thus
    "HTTP:Content-Length".

    There are also "global attributes", which may be produced by
    different MCCs depending on the configuration. The keys of such
    attributes are NOT prefixed by the name of the producing
    MCC. Before any new global attribute is introduced, it must be
    agreed upon by the core development team and added below. The
    global attributes decided so far are:
    \li \c Request-URI Identifies the service to which the message
    shall be sent. This attribute is produced by e.g. the HTTP MCC
    and used by the plexer for routing the message to the
    appropriate service.
    */
  class MessageAttributes {
  public:

    //! The default constructor.
    /*! This is the default constructor of the MessageAttributes
      class. It constructs an empty object that initially contains no
      attributes.
     */
    MessageAttributes();

    //! Sets a unique value of an attribute.
    /*! This method removes any previous value of an attribute and
      sets the new value as the only value.
      \param key The key of the attribute.
      \param value The (new) value of the attribute.
     */
    void set(const std::string& key, const std::string& value);

    //! Adds a value to an attribute.
    /*! This method adds a new value to an attribute. Any previous
      value will be preserved, i.e. the attribute may become multiple
      valued.
      \param key The key of the attribute.
      \param value The (new) value of the attribute.
     */
    void add(const std::string& key, const std::string& value);

    //! Removes all attributes with a certain key.
    /*! This method removes all attributes that match a certain key.
      \param key The key of the attributes to remove.
     */
    void removeAll(const std::string& key);

    //! Removes one value of an attribute.
    /*! This method removes a certain value from the attribute that
      matches a certain key.
      \param key The key of the attribute from which the value shall
      be removed.
      \param value The value to remove.
     */
    void remove(const std::string& key, const std::string& value);

    //! Returns the number of values of an attribute.
    /*! Returns the number of values of an attribute that matches a
      certain key.
      \param key The key of the attribute for which to count values.
      \return The number of values that corresponds to the key.
    */
    int count(const std::string& key) const;

    //! Returns the value of a single-valued attribute.
    /*! This method returns the value of a single-valued attribute. If
      the attribute is not single valued (i.e. there is no such
      attribute or it is a multiple-valued attribute) an empty string
      is returned.
      \param key The key of the attribute for which to return the
      value.
      \return The value of the attribute.
     */
    const std::string& get(const std::string& key) const;

    //! Access the value(s) of an attribute.
    /*! This method returns an AttributeIterator that can be used to
      access the values of an attribute.
      \param key The key of the attribute for which to return the
      values.
      \return An AttributeIterator for access of the values of the
      attribute.
     */    
    AttributeIterator getAll(const std::string& key) const;

  protected:

    //! Internal storage of attributes.
    /*! An AttrMap (multimap) in which all attributes (key-value
      pairs) are stored.
     */
    AttrMap attributes_;

  };

}

#endif
