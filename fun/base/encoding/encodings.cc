#include "fun/base/encoding/encodings.h"
#include "fun/base/encoding/iso8859_10_encoding.h"
#include "fun/base/encoding/iso8859_11_encoding.h"
#include "fun/base/encoding/iso8859_13_encoding.h"
#include "fun/base/encoding/iso8859_14_encoding.h"
#include "fun/base/encoding/iso8859_16_encoding.h"
#include "fun/base/encoding/iso8859_3_encoding.h"
#include "fun/base/encoding/iso8859_4_encoding.h"
#include "fun/base/encoding/iso8859_5_encoding.h"
#include "fun/base/encoding/iso8859_6_encoding.h"
#include "fun/base/encoding/iso8859_7_encoding.h"
#include "fun/base/encoding/iso8859_8_encoding.h"
#include "fun/base/encoding/iso8859_9_encoding.h"
#include "fun/base/encoding/text_encoding.h"
#include "fun/base/encoding/windows1253_encoding.h"
#include "fun/base/encoding/windows1254_encoding.h"
#include "fun/base/encoding/windows1255_encoding.h"
#include "fun/base/encoding/windows1256_encoding.h"
#include "fun/base/encoding/windows1257_encoding.h"
#include "fun/base/encoding/windows1258_encoding.h"
#include "fun/base/encoding/windows874_encoding.h"
#include "fun/base/encoding/windows932_encoding.h"
#include "fun/base/encoding/windows936_encoding.h"
#include "fun/base/encoding/windows949_encoding.h"
#include "fun/base/encoding/windows950_encoding.h"

namespace fun {

void RegisterExtraEncodings() {
  TextEncoding::Add(new ISO8859_10_Encoding);
  TextEncoding::Add(new ISO8859_11_Encoding);
  TextEncoding::Add(new ISO8859_13_Encoding);
  TextEncoding::Add(new ISO8859_14_Encoding);
  TextEncoding::Add(new ISO8859_16_Encoding);
  TextEncoding::Add(new ISO8859_3_Encoding);
  TextEncoding::Add(new ISO8859_4_Encoding);
  TextEncoding::Add(new ISO8859_5_Encoding);
  TextEncoding::Add(new ISO8859_6_Encoding);
  TextEncoding::Add(new ISO8859_7_Encoding);
  TextEncoding::Add(new ISO8859_8_Encoding);
  TextEncoding::Add(new ISO8859_9_Encoding);
  TextEncoding::Add(new Windows1253_Encoding);
  TextEncoding::Add(new Windows1254_Encoding);
  TextEncoding::Add(new Windows1255_Encoding);
  TextEncoding::Add(new Windows1256_Encoding);
  TextEncoding::Add(new Windows1257_Encoding);
  TextEncoding::Add(new Windows1258_Encoding);
  TextEncoding::Add(new Windows874_Encoding);
  TextEncoding::Add(new Windows932_Encoding);
  TextEncoding::Add(new Windows936_Encoding);
  TextEncoding::Add(new Windows949_Encoding);
  TextEncoding::Add(new Windows950_Encoding);
}

}  // namespace fun
