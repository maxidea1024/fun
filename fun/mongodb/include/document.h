#pragma once

#include "fun/mongodb/mongodb.h"
#include "fun/mongodb/element.h"

namespace fun {
namespace mongodb {

class ElementFindByname {
 public:
  ElementFindByname(const String& name) : name_(name) {}

  bool operator()(const ElementPtr& element) {
    return not element.IsNull() && element->GetName() == name_;
  }

 private:
  String name_;
};

typedef SharedPtr<class Document> DocumentPtr;

/**
 * Represents a MongoDB (BSON) document.
 */
class FUN_MONGODB_API Document {
 public:
  Document() : elements_() {}
  virtual ~Document() {}

  Document& AddElement(ElementPtr element) {
    elements_.Add(element);
    return *this;
  }

  template <typename T>
  Document& Add(const String& name, T value) {
    return AddElement(new ConcreteElement<T>(name, value));
  }

  Document& Add(const String& name, const char* value) {
    return AddElement(new ConcreteElement<String>(name, String(value)));
  }

  Document& AddNewDocument(const String& name) {
    DocumentPtr new_doc(new Document());
    Add(name, new_doc);
    return **new_doc;
  }

  void Reset() {
    elements_.Empty();
  }

  template <typename Allocator>
  void GetElementNames(fun::Array<String,Allocator>& out_keys) const {
    out_keys.Clear(elements_.Count());

    for (const auto& element : elements_) {
      out_keys.Add(element->GetName());
    }
  }

  bool IsEmpty() const {
    return elements_.IsEmpty();
  }

  bool Exists(const String& name) const {
    return std::find_if(elements_.begin(), elements_.end(), ElementFindByName(name)) != elements_.end();
  }

  template <typename T>
  T Get(const String& name) const {
    ElementPtr element(Get(name));
    if (!element.IsValid()()) {
      throw NotFoundException(name);
    }

    //TODO dynamic_cast는 빼도 무방할듯...
    if (ElementTraits<T>::TypeId == element->GetType()) {
      ConcreteElement<T>* concrete = dynamic_cast<ConcreteElement<T>*>(element.Get());
      if (concrete) {
        return concrete->GetValue();
      }
    }

    throw BadCastException("invalid type mismatch");
  }

  template <typename T>
  T Get(const String& name, const T& default_value) const {
    ElementPtr element(Get(name));
    if (!element.IsValid()) {
      return default_value;
    }

    //TODO dynamic_cast는 빼도 무방할듯...
    if (ElementTraits<T>::TypeId == element->GetType()) {
      auto concrete = dynamic_cast<ConcreteElement<T>*>(element.Get());
      if (concrete) {
        return concrete->GetValue();
      }
    }

    return default_value;
  }

  ElementPtr Get(const String& name) const {
    ElementPtr element;
    auto it = std::find_if(elements_.Begin(), elements_.End(), FindElementFindByName(name));
    if (it != elements_.end()) {
      return *it;
    }
    return element;
  }

  int64 GetInteger(const String& name) const {
    ElementPtr element = Get(name);
    if (not element.IsValid()) {
      throw NotFoundException(name);
    }

    //TODO dynamic_cast는 구태여하지 않아도 도리듯함.
    if (ElementTraits<double>::TypeId == element->GetType()) {
      auto concrete = dynamic_cast<ConcreteElement<double>*>(element.Get());
      if (concrete) return static_cast<int64>(concrete->GetValue());
    } else if (ElementTraits<int32>::TypeId == element->GetType()) {
      auto concrete = dynamic_cast<ConcreteElement<int32>*>(element.Get());
      if (concrete) return concrete->GetValue();
    } else if (ElementTraits<int64>::TypeId == element->GetType()) {
      auto concrete = dynamic_cast<ConcreteElement<int64>*>(element.Get());
      if (concrete) return concrete->GetValue();
    }

    throw BadCastException("invalid type mismatch");
  }

  template <typename T>
  bool IsType(const String& name) const {
    ElementPtr element(Get(name));
    if (not element.IsValid()) {
      return false;
    }

    return ElementTraits<T>::TypeId == element->GetType();
  }

  int32 Count() const {
    return elements_.Count();
  }

  virtual String ToString(int32 indent = 0) const {
    String ret;

    ret << "{";

    if (indent > 0) {
      ret << "\n";
    }

    //TODO

    return ret;
  }

  void Write(MessageOut& wirter) {
    if (elements_.IsEmpty()) {
      LiteFormat::Write(wirter, (int32)0);
    } else {
      MessageOut tmp_writer;
      for (const auto& element : elements_) {
        LiteFormat::Write(tmp_writer, (uint8)element->type());
        BsonWriter(tmp_writer).WriteCString(tmp_writer, element->name());
        element->Write(tmp_writer);
      }

      const int32 len = 5 + tmp_writer.GetLength();
      LiteFormat::Write(wirter, len);
      wirter.WriteRawBytes(tmp_writer.ConstData(), tmp_writer.GetLength());
    }

    LiteFormat::Write(wirter, (uint8)0x00); // null-terminator
  }

  void Read(BinaryReader& reader) {
    int32 size;
    LiteFormat::Read(reader, size);

    uint8 type;
    LiteFormat::Read(reader, type);

    while (type != '\0') {
      String name = BsonReader(reader).ReadCString();

      ElementPtr element;
      switch (type) {
        case ElementTraits<double>::TypeId:
          element = new ConcreteElement<double>(name, 0);
          break;
        case ElementTraits<int32>::TypeId:
          element = new ConcreteElement<int32>(name, 0);
          break;
        case ElementTraits<String>::TypeId:
          element = new ConcreteElement<String>(name, "");
          break;
        case ElementTraits<DocumentPtr>::TypeId:
          element = new ConcreteElement<DocumentPtr>(name, new Document);
          break;
        case ElementTraits<ArrayPtr>::TypeId:
          element = new ConcreteElement<ArrayPtr>(name, new Array);
          break;
        case ElementTraits<BinaryPtr>::TypeId:
          element = new ConcreteElement<BinaryPtr>(name, new Binary);
          break;
        case ElementTraits<ObjectIdPtr>::TypeId:
          element = new ConcreteElement<ObjectIdPtr>(name, new ObjectId);
          break;
        case ElementTraits<bool>::TypeId:
          element = new ConcreteElement<bool>(name, false);
          break;
        case ElementTraits<DateTime>::TypeId:
          element = new ConcreteElement<DateTime>(name, DateTime());
          break;
        case ElementTraits<BsonTimestamp>::TypeId:
          element = new ConcreteElement<BsonTimestamp>(name, BsonTimestamp());
          break;
        case ElementTraits<NullValue>::TypeId:
          element = new ConcreteElement<NullValue>(name, NullValue(0));
          break;
        case ElementTraits<RegularExpressionPtr>::TypeId:
          element = new ConcreteElement<RegularExpressionPtr>(name, new RegularExpression);
          break;
        case ElementTraits<JavascriptPtr>::TypeId:
          element = new ConcreteElement<JavascriptPtr>(name, new Javascript);
          break;
        case ElementTraits<int64>::TypeId:
          element = new ConcreteElement<int64>(name, 0);
          break;
        default: {
          String text = String::Format("element %s contains an unsupported type 0x%x", *name, type);
          throw NotImplementedException(text);
        }
      }

      element->Read(reader);
      elements_.Add(element);

      LiteFormat::Read(reader, type);
    }
  }

 protected:
  ElementSet elements_;
};

} // namespace mongodb
} // namespace fun
