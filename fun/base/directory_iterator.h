#pragma once

#include "fun/base/base.h"
#include "fun/base/file.h"
#include "fun/base/path.h"

namespace fun {

class DirectoryIteratorImpl;

/**
 * The DirectoryIterator class is used to enumerate
 * all files in a directory.
 * 
 * DirectoryIterator has some limitations:
 *   * only forward iteration (++) is supported
 *   * an iterator copied from another one will always
 *     point to the same file as the original iterator,
 *     even is the original iterator has been advanced
 *     (all copies of an iterator share their state with
 *     the original iterator)
 *   * because of this you should only use the prefix
 *     increment operator
 */
class FUN_BASE_API DirectoryIterator {
 public:
  /**
   * Creates the end iterator.
   */
  DirectoryIterator();

  /**
   * Creates a directory iterator for the given path.
   */
  DirectoryIterator(const String& path);

  /**
   * Creates a directory iterator for the given path.
   */
  DirectoryIterator(const DirectoryIterator& iterator);

  /**
   * Creates a directory iterator for the given file.
   */
  DirectoryIterator(const File& file);

  /**
   * Creates a directory iterator for the given path.
   */
  DirectoryIterator(const Path& path);

  /**
   * Destroys the DirectoryIterator.
   */
  virtual ~DirectoryIterator();

  /**
   * Returns the current filename.
   */
  const String& GetName() const;

  /**
   * Returns the current path.
   */
  const Path& GetPath() const;

  DirectoryIterator& operator = (const DirectoryIterator& iterator);
  DirectoryIterator& operator = (const File& file);
  DirectoryIterator& operator = (const Path& path);
  DirectoryIterator& operator = (const String& path);

  virtual DirectoryIterator& operator ++ (); // prefix

  //@ deprecated
  /**
   * Please use the prefix increment operator instead.
   */
  DirectoryIterator operator ++ (int); // postfix

  const File& operator * () const;
  File& operator * ();
  const File* operator -> () const;
  File* operator -> ();

  bool operator == (const DirectoryIterator& other) const;
  bool operator != (const DirectoryIterator& other) const;

 protected:
  Path path_;
  File file_;

 private:
  //TODO 구태여 참조카운팅을 해야할까??
  DirectoryIteratorImpl* impl_;
};


//
// inlines
//

FUN_ALWAYS_INLINE const String& DirectoryIterator::GetName() const {
  return path_.GetFileName();
}

FUN_ALWAYS_INLINE const Path& DirectoryIterator::GetPath() const {
  return path_;
}

FUN_ALWAYS_INLINE const File& DirectoryIterator::operator * () const {
  return file_;
}

FUN_ALWAYS_INLINE File& DirectoryIterator::operator * () {
  return file_;
}

FUN_ALWAYS_INLINE const File* DirectoryIterator::operator -> () const {
  return &file_;
}

FUN_ALWAYS_INLINE File* DirectoryIterator::operator -> () {
  return &file_;
}

FUN_ALWAYS_INLINE bool DirectoryIterator::operator == (const DirectoryIterator& other) const {
  return GetName() == other.GetName();
}

FUN_ALWAYS_INLINE bool DirectoryIterator::operator != (const DirectoryIterator& other) const {
  return GetName() != other.GetName();
}

} // namespace fun
