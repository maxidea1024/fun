#pragma once

//#define FUN_NO_SYSTEM_LOCALE  1

#include "fun/base/base.h"
#include "fun/base/date_time_types.h"
#include "fun/base/scoped_ptr.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API Locale
{
 public:
  enum Language {
    AnyLanguage = 0,
    C = 1, // english (US)
    Abkhazian = 2,
    Oromo = 3,
    Afar = 4,
    Afrikaans = 5,
    Albanian = 6,
    Amharic = 7,
    Arabic = 8,
    Armenian = 9,
    Assamese = 10,
    Aymara = 11,
    Azerbaijani = 12,
    Bashkir = 13,
    Basque = 14,
    Bengali = 15,
    Dzongkha = 16,
    Bihari = 17,
    Bislama = 18,
    Breton = 19,
    Bulgarian = 20,
    Burmese = 21,
    Belarusian = 22,
    Khmer = 23,
    Catalan = 24,
    Chinese = 25,
    Corsican = 26,
    Croatian = 27,
    Czech = 28,
    Danish = 29,
    Dutch = 30,
    English = 31,
    Esperanto = 32,
    Estonian = 33,
    Faroese = 34,
    Fijian = 35,
    Finnish = 36,
    French = 37,
    WesternFrisian = 38,
    Gaelic = 39,
    Galician = 40,
    Georgian = 41,
    German = 42,
    Greek = 43,
    Greenlandic = 44,
    Guarani = 45,
    Gujarati = 46,
    Hausa = 47,
    Hebrew = 48,
    Hindi = 49,
    Hungarian = 50,
    Icelandic = 51,
    Indonesian = 52,
    Interlingua = 53,
    Interlingue = 54,
    Inuktitut = 55,
    Inupiak = 56,
    Irish = 57,
    Italian = 58,
    Japanese = 59,
    Javanese = 60,
    Kannada = 61,
    Kashmiri = 62,
    Kazakh = 63,
    Kinyarwanda = 64,
    Kirghiz = 65,
    Korean = 66,
    Kurdish = 67,
    Rundi = 68,
    Lao = 69,
    Latin = 70,
    Latvian = 71,
    Lingala = 72,
    Lithuanian = 73,
    Macedonian = 74,
    Malagasy = 75,
    Malay = 76,
    Malayalam = 77,
    Maltese = 78,
    Maori = 79,
    Marathi = 80,
    Marshallese = 81,
    Mongolian = 82,
    NauruLanguage = 83,
    Nepali = 84,
    NorwegianBokmal = 85,
    Occitan = 86,
    Oriya = 87,
    Pashto = 88,
    Persian = 89,
    Polish = 90,
    Portuguese = 91,
    Punjabi = 92,
    Quechua = 93,
    Romansh = 94,
    Romanian = 95,
    Russian = 96,
    Samoan = 97,
    Sango = 98,
    Sanskrit = 99,
    Serbian = 100,
    Ossetic = 101,
    SouthernSotho = 102,
    Tswana = 103,
    Shona = 104,
    Sindhi = 105,
    Sinhala = 106,
    Swati = 107,
    Slovak = 108,
    Slovenian = 109,
    Somali = 110,
    Spanish = 111,
    Sundanese = 112,
    Swahili = 113,
    Swedish = 114,
    Sardinian = 115,
    Tajik = 116,
    Tamil = 117,
    Tatar = 118,
    Telugu = 119,
    Thai = 120,
    Tibetan = 121,
    Tigrinya = 122,
    Tongan = 123,
    Tsonga = 124,
    Turkish = 125,
    Turkmen = 126,
    Tahitian = 127,
    Uighur = 128,
    Ukrainian = 129,
    Urdu = 130,
    Uzbek = 131,
    Vietnamese = 132,
    Volapuk = 133,
    Welsh = 134,
    Wolof = 135,
    Xhosa = 136,
    Yiddish = 137,
    Yoruba = 138,
    Zhuang = 139,
    Zulu = 140,
    NorwegianNynorsk = 141,
    Bosnian = 142,
    Divehi = 143,
    Manx = 144,
    Cornish = 145,
    Akan = 146,
    Konkani = 147,
    Ga = 148,
    Igbo = 149,
    Kamba = 150,
    Syriac = 151,
    Blin = 152,
    Geez = 153,
    Koro = 154,
    Sidamo = 155,
    Atsam = 156,
    Tigre = 157,
    Jju = 158,
    Friulian = 159,
    Venda = 160,
    Ewe = 161,
    Walamo = 162,
    Hawaiian = 163,
    Tyap = 164,
    Nyanja = 165,
    Filipino = 166,
    SwissGerman = 167,
    SichuanYi = 168,
    Kpelle = 169,
    LowGerman = 170,
    SouthNdebele = 171,
    NorthernSotho = 172,
    NorthernSami = 173,
    Taroko = 174,
    Gusii = 175,
    Taita = 176,
    Fulah = 177,
    Kikuyu = 178,
    Samburu = 179,
    Sena = 180,
    NorthNdebele = 181,
    Rombo = 182,
    Tachelhit = 183,
    Kabyle = 184,
    Nyankole = 185,
    Bena = 186,
    Vunjo = 187,
    Bambara = 188,
    Embu = 189,
    Cherokee = 190,
    Morisyen = 191,
    Makonde = 192,
    Langi = 193,
    Ganda = 194,
    Bemba = 195,
    Kabuverdianu = 196,
    Meru = 197,
    Kalenjin = 198,
    Nama = 199,
    Machame = 200,
    Colognian = 201,
    Masai = 202,
    Soga = 203,
    Luyia = 204,
    Asu = 205,
    Teso = 206,
    Saho = 207,
    KoyraChiini = 208,
    Rwa = 209,
    Luo = 210,
    Chiga = 211,
    CentralMoroccoTamazight = 212,
    KoyraboroSenni = 213,
    Shambala = 214,
    Bodo = 215,
    Avaric = 216,
    Chamorro = 217,
    Chechen = 218,
    Church = 219,
    Chuvash = 220,
    Cree = 221,
    Haitian = 222,
    Herero = 223,
    HiriMotu = 224,
    Kanuri = 225,
    Komi = 226,
    Kongo = 227,
    Kwanyama = 228,
    Limburgish = 229,
    LubaKatanga = 230,
    Luxembourgish = 231,
    Navaho = 232,
    Ndonga = 233,
    Ojibwa = 234,
    Pali = 235,
    Walloon = 236,
    Aghem = 237,
    Basaa = 238,
    Zarma = 239,
    Duala = 240,
    JolaFonyi = 241,
    Ewondo = 242,
    Bafia = 243,
    MakhuwaMeetto = 244,
    Mundang = 245,
    Kwasio = 246,
    Nuer = 247,
    Sakha = 248,
    Sangu = 249,
    CongoSwahili = 250,
    Tasawaq = 251,
    Vai = 252,
    Walser = 253,
    Yangben = 254,
    Avestan = 255,
    Asturian = 256,
    Ngomba = 257,
    Kako = 258,
    Meta = 259,
    Ngiemboon = 260,
    Aragonese = 261,
    Akkadian = 262,
    AncientEgyptian = 263,
    AncientGreek = 264,
    Aramaic = 265,
    Balinese = 266,
    Bamun = 267,
    BatakToba = 268,
    Buginese = 269,
    Buhid = 270,
    Carian = 271,
    Chakma = 272,
    ClassicalMandaic = 273,
    Coptic = 274,
    Dogri = 275,
    EasternCham = 276,
    EasternKayah = 277,
    Etruscan = 278,
    Gothic = 279,
    Hanunoo = 280,
    Ingush = 281,
    LargeFloweryMiao = 282,
    Lepcha = 283,
    Limbu = 284,
    Lisu = 285,
    Lu = 286,
    Lycian = 287,
    Lydian = 288,
    Mandingo = 289,
    Manipuri = 290,
    Meroitic = 291,
    NorthernThai = 292,
    OldIrish = 293,
    OldNorse = 294,
    OldPersian = 295,
    OldTurkish = 296,
    Pahlavi = 297,
    Parthian = 298,
    Phoenician = 299,
    PrakritLanguage = 300,
    Rejang = 301,
    Sabaean = 302,
    Samaritan = 303,
    Santali = 304,
    Saurashtra = 305,
    Sora = 306,
    Sylheti = 307,
    Tagbanwa = 308,
    TaiDam = 309,
    TaiNua = 310,
    Ugaritic = 311,
    Akoose = 312,
    Lakota = 313,
    StandardMoroccanTamazight = 314,
    Mapuche = 315,
    CentralKurdish = 316,
    LowerSorbian = 317,
    UpperSorbian = 318,
    Kenyang = 319,
    Mohawk = 320,
    Nko = 321,
    Prussian = 322,
    Kiche = 323,
    SouthernSami = 324,
    LuleSami = 325,
    InariSami = 326,
    SkoltSami = 327,
    Warlpiri = 328,
    ManichaeanMiddlePersian = 329,
    Mende = 330,
    AncientNorthArabian = 331,
    LinearA = 332,
    HmongNjua = 333,
    Ho = 334,
    Lezghian = 335,
    Bassa = 336,
    Mono = 337,
    TedimChin = 338,
    Maithili = 339,
    Ahom = 340,
    AmericanSignLanguage = 341,
    ArdhamagadhiPrakrit = 342,
    Bhojpuri = 343,
    HieroglyphicLuwian = 344,
    LiteraryChinese = 345,
    Mazanderani = 346,
    Mru = 347,
    Newari = 348,
    NorthernLuri = 349,
    Palauan = 350,
    Papiamento = 351,
    Saraiki = 352,
    TokelauLanguage = 353,
    TokPisin = 354,
    TuvaluLanguage = 355,
    UncodedLanguages = 356,
    Cantonese = 357,
    Osage = 358,
    Tangut = 359,

    Norwegian = NorwegianBokmal,
    Moldavian = Romanian,
    SerboCroatian = Serbian,
    Tagalog = Filipino,
    Twi = Akan,
    Afan = Oromo,
    Byelorussian = Belarusian,
    Bhutani = Dzongkha,
    Cambodian = Khmer,
    Kurundi = Rundi,
    RhaetoRomance = Romansh,
    Chewa = Nyanja,
    Frisian = WesternFrisian,
    Uigur = Uighur,

    LastLanguage = Tangut
  };

  enum Script {
    AnyScript = 0,
    ArabicScript = 1,
    CyrillicScript = 2,
    DeseretScript = 3,
    GurmukhiScript = 4,
    SimplifiedHanScript = 5,
    TraditionalHanScript = 6,
    LatinScript = 7,
    MongolianScript = 8,
    TifinaghScript = 9,
    ArmenianScript = 10,
    BengaliScript = 11,
    CherokeeScript = 12,
    DevanagariScript = 13,
    EthiopicScript = 14,
    GeorgianScript = 15,
    GreekScript = 16,
    GujaratiScript = 17,
    HebrewScript = 18,
    JapaneseScript = 19,
    KhmerScript = 20,
    KannadaScript = 21,
    KoreanScript = 22,
    LaoScript = 23,
    MalayalamScript = 24,
    MyanmarScript = 25,
    OriyaScript = 26,
    TamilScript = 27,
    TeluguScript = 28,
    ThaanaScript = 29,
    ThaiScript = 30,
    TibetanScript = 31,
    SinhalaScript = 32,
    SyriacScript = 33,
    YiScript = 34,
    VaiScript = 35,
    AvestanScript = 36,
    BalineseScript = 37,
    BamumScript = 38,
    BatakScript = 39,
    BopomofoScript = 40,
    BrahmiScript = 41,
    BugineseScript = 42,
    BuhidScript = 43,
    CanadianAboriginalScript = 44,
    CarianScript = 45,
    ChakmaScript = 46,
    ChamScript = 47,
    CopticScript = 48,
    CypriotScript = 49,
    EgyptianHieroglyphsScript = 50,
    FraserScript = 51,
    GlagoliticScript = 52,
    GothicScript = 53,
    HanScript = 54,
    HangulScript = 55,
    HanunooScript = 56,
    ImperialAramaicScript = 57,
    InscriptionalPahlaviScript = 58,
    InscriptionalParthianScript = 59,
    JavaneseScript = 60,
    KaithiScript = 61,
    KatakanaScript = 62,
    KayahLiScript = 63,
    KharoshthiScript = 64,
    LannaScript = 65,
    LepchaScript = 66,
    LimbuScript = 67,
    LinearBScript = 68,
    LycianScript = 69,
    LydianScript = 70,
    MandaeanScript = 71,
    MeiteiMayekScript = 72,
    MeroiticScript = 73,
    MeroiticCursiveScript = 74,
    NkoScript = 75,
    NewTaiLueScript = 76,
    OghamScript = 77,
    OlChikiScript = 78,
    OldItalicScript = 79,
    OldPersianScript = 80,
    OldSouthArabianScript = 81,
    OrkhonScript = 82,
    OsmanyaScript = 83,
    PhagsPaScript = 84,
    PhoenicianScript = 85,
    PollardPhoneticScript = 86,
    RejangScript = 87,
    RunicScript = 88,
    SamaritanScript = 89,
    SaurashtraScript = 90,
    SharadaScript = 91,
    ShavianScript = 92,
    SoraSompengScript = 93,
    CuneiformScript = 94,
    SundaneseScript = 95,
    SylotiNagriScript = 96,
    TagalogScript = 97,
    TagbanwaScript = 98,
    TaiLeScript = 99,
    TaiVietScript = 100,
    TakriScript = 101,
    UgariticScript = 102,
    BrailleScript = 103,
    HiraganaScript = 104,
    CaucasianAlbanianScript = 105,
    BassaVahScript = 106,
    DuployanScript = 107,
    ElbasanScript = 108,
    GranthaScript = 109,
    PahawhHmongScript = 110,
    KhojkiScript = 111,
    LinearAScript = 112,
    MahajaniScript = 113,
    ManichaeanScript = 114,
    MendeKikakuiScript = 115,
    ModiScript = 116,
    MroScript = 117,
    OldNorthArabianScript = 118,
    NabataeanScript = 119,
    PalmyreneScript = 120,
    PauCinHauScript = 121,
    OldPermicScript = 122,
    PsalterPahlaviScript = 123,
    SiddhamScript = 124,
    KhudawadiScript = 125,
    TirhutaScript = 126,
    VarangKshitiScript = 127,
    AhomScript = 128,
    AnatolianHieroglyphsScript = 129,
    HatranScript = 130,
    MultaniScript = 131,
    OldHungarianScript = 132,
    SignWritingScript = 133,
    AdlamScript = 134,
    BhaiksukiScript = 135,
    MarchenScript = 136,
    NewaScript = 137,
    OsageScript = 138,
    TangutScript = 139,
    HanWithBopomofoScript = 140,
    JamoScript = 141,

    SimplifiedChineseScript = SimplifiedHanScript,
    TraditionalChineseScript = TraditionalHanScript,

    LastScript = JamoScript
  };

  enum Country {
    AnyCountry = 0,
    Afghanistan = 1,
    Albania = 2,
    Algeria = 3,
    AmericanSamoa = 4,
    Andorra = 5,
    Angola = 6,
    Anguilla = 7,
    Antarctica = 8,
    AntiguaAndBarbuda = 9,
    Argentina = 10,
    Armenia = 11,
    Aruba = 12,
    Australia = 13,
    Austria = 14,
    Azerbaijan = 15,
    Bahamas = 16,
    Bahrain = 17,
    Bangladesh = 18,
    Barbados = 19,
    Belarus = 20,
    Belgium = 21,
    Belize = 22,
    Benin = 23,
    Bermuda = 24,
    Bhutan = 25,
    Bolivia = 26,
    BosniaAndHerzegowina = 27,
    Botswana = 28,
    BouvetIsland = 29,
    Brazil = 30,
    BritishIndianOceanTerritory = 31,
    Brunei = 32,
    Bulgaria = 33,
    BurkinaFaso = 34,
    Burundi = 35,
    Cambodia = 36,
    Cameroon = 37,
    Canada = 38,
    CapeVerde = 39,
    CaymanIslands = 40,
    CentralAfricanRepublic = 41,
    Chad = 42,
    Chile = 43,
    China = 44,
    ChristmasIsland = 45,
    CocosIslands = 46,
    Colombia = 47,
    Comoros = 48,
    CongoKinshasa = 49,
    CongoBrazzaville = 50,
    CookIslands = 51,
    CostaRica = 52,
    IvoryCoast = 53,
    Croatia = 54,
    Cuba = 55,
    Cyprus = 56,
    CzechRepublic = 57,
    Denmark = 58,
    Djibouti = 59,
    Dominica = 60,
    DominicanRepublic = 61,
    EastTimor = 62,
    Ecuador = 63,
    Egypt = 64,
    ElSalvador = 65,
    EquatorialGuinea = 66,
    Eritrea = 67,
    Estonia = 68,
    Ethiopia = 69,
    FalklandIslands = 70,
    FaroeIslands = 71,
    Fiji = 72,
    Finland = 73,
    France = 74,
    Guernsey = 75,
    FrenchGuiana = 76,
    FrenchPolynesia = 77,
    FrenchSouthernTerritories = 78,
    Gabon = 79,
    Gambia = 80,
    Georgia = 81,
    Germany = 82,
    Ghana = 83,
    Gibraltar = 84,
    Greece = 85,
    Greenland = 86,
    Grenada = 87,
    Guadeloupe = 88,
    Guam = 89,
    Guatemala = 90,
    Guinea = 91,
    GuineaBissau = 92,
    Guyana = 93,
    Haiti = 94,
    HeardAndMcDonaldIslands = 95,
    Honduras = 96,
    HongKong = 97,
    Hungary = 98,
    Iceland = 99,
    India = 100,
    Indonesia = 101,
    Iran = 102,
    Iraq = 103,
    Ireland = 104,
    Israel = 105,
    Italy = 106,
    Jamaica = 107,
    Japan = 108,
    Jordan = 109,
    Kazakhstan = 110,
    Kenya = 111,
    Kiribati = 112,
    NorthKorea = 113,
    SouthKorea = 114,
    Kuwait = 115,
    Kyrgyzstan = 116,
    Laos = 117,
    Latvia = 118,
    Lebanon = 119,
    Lesotho = 120,
    Liberia = 121,
    Libya = 122,
    Liechtenstein = 123,
    Lithuania = 124,
    Luxembourg = 125,
    Macau = 126,
    Macedonia = 127,
    Madagascar = 128,
    Malawi = 129,
    Malaysia = 130,
    Maldives = 131,
    Mali = 132,
    Malta = 133,
    MarshallIslands = 134,
    Martinique = 135,
    Mauritania = 136,
    Mauritius = 137,
    Mayotte = 138,
    Mexico = 139,
    Micronesia = 140,
    Moldova = 141,
    Monaco = 142,
    Mongolia = 143,
    Montserrat = 144,
    Morocco = 145,
    Mozambique = 146,
    Myanmar = 147,
    Namibia = 148,
    NauruCountry = 149,
    Nepal = 150,
    Netherlands = 151,
    CuraSao = 152,
    NewCaledonia = 153,
    NewZealand = 154,
    Nicaragua = 155,
    Niger = 156,
    Nigeria = 157,
    Niue = 158,
    NorfolkIsland = 159,
    NorthernMarianaIslands = 160,
    Norway = 161,
    Oman = 162,
    Pakistan = 163,
    Palau = 164,
    PalestinianTerritories = 165,
    Panama = 166,
    PapuaNewGuinea = 167,
    Paraguay = 168,
    Peru = 169,
    Philippines = 170,
    Pitcairn = 171,
    Poland = 172,
    Portugal = 173,
    PuertoRico = 174,
    Qatar = 175,
    Reunion = 176,
    Romania = 177,
    Russia = 178,
    Rwanda = 179,
    SaintKittsAndNevis = 180,
    SaintLucia = 181,
    SaintVincentAndTheGrenadines = 182,
    Samoa = 183,
    SanMarino = 184,
    SaoTomeAndPrincipe = 185,
    SaudiArabia = 186,
    Senegal = 187,
    Seychelles = 188,
    SierraLeone = 189,
    Singapore = 190,
    Slovakia = 191,
    Slovenia = 192,
    SolomonIslands = 193,
    Somalia = 194,
    SouthAfrica = 195,
    SouthGeorgiaAndTheSouthSandwichIslands = 196,
    Spain = 197,
    SriLanka = 198,
    SaintHelena = 199,
    SaintPierreAndMiquelon = 200,
    Sudan = 201,
    Suriname = 202,
    SvalbardAndJanMayenIslands = 203,
    Swaziland = 204,
    Sweden = 205,
    Switzerland = 206,
    Syria = 207,
    Taiwan = 208,
    Tajikistan = 209,
    Tanzania = 210,
    Thailand = 211,
    Togo = 212,
    TokelauCountry = 213,
    Tonga = 214,
    TrinidadAndTobago = 215,
    Tunisia = 216,
    Turkey = 217,
    Turkmenistan = 218,
    TurksAndCaicosIslands = 219,
    TuvaluCountry = 220,
    Uganda = 221,
    Ukraine = 222,
    UnitedArabEmirates = 223,
    UnitedKingdom = 224,
    UnitedStates = 225,
    UnitedStatesMinorOutlyingIslands = 226,
    Uruguay = 227,
    Uzbekistan = 228,
    Vanuatu = 229,
    VaticanCityState = 230,
    Venezuela = 231,
    Vietnam = 232,
    BritishVirginIslands = 233,
    UnitedStatesVirginIslands = 234,
    WallisAndFutunaIslands = 235,
    WesternSahara = 236,
    Yemen = 237,
    CanaryIslands = 238,
    Zambia = 239,
    Zimbabwe = 240,
    ClippertonIsland = 241,
    Montenegro = 242,
    Serbia = 243,
    SaintBarthelemy = 244,
    SaintMartin = 245,
    LatinAmericaAndTheCaribbean = 246,
    AscensionIsland = 247,
    AlandIslands = 248,
    DiegoGarcia = 249,
    CeutaAndMelilla = 250,
    IsleOfMan = 251,
    Jersey = 252,
    TristanDaCunha = 253,
    SouthSudan = 254,
    Bonaire = 255,
    SintMaarten = 256,
    Kosovo = 257,
    EuropeanUnion = 258,
    OutlyingOceania = 259,

    Tokelau = TokelauCountry,
    Tuvalu = TuvaluCountry,
    DemocraticRepublicOfCongo = CongoKinshasa,
    PeoplesRepublicOfCongo = CongoBrazzaville,
    DemocraticRepublicOfKorea = NorthKorea,
    RepublicOfKorea = SouthKorea,
    RussianFederation = Russia,
    SyrianArabRepublic = Syria,

    LastCountry = OutlyingOceania
  };

  enum MeasurementSystem {
    MetricSystem,
    ImperialUSSystem,
    ImperialUKSystem,
    ImperialSystem = ImperialUSSystem
  };

  enum FormatType {
    LongFormat,
    ShortFormat,
    NarrowFormat,
  };

  enum NumberOptions {
    DefaultNumberOptions = 0x0,
    OmitGroupSeparator = 0x01,
    RejectGroupSeparator = 0x02,
    OmitLeadingZeroInExponent = 0x04,
    RejectLeadingZeroInExponent = 0x08,
    IncludeTrailingZeroesAfterDot = 0x10,
    RejectTrailingZeroesAfterDot = 0x20
  };

  enum FloatingPointPrecisionOption {
    FloatingPointShortest = -128
  };

  //TODO 이름 충돌때문에 어쩔수 없이 앞에 'CSF'를 붙여 주었지만, 정리해서 'CSF'를 빼주도록 하자.
  enum CurrencySymbolFormat {
    CSFCurrencyIsoCode,
    CSFCurrencySymbol,
    //TODO 현재 시스템 로케일이 아닐 경우, 테이블이 NativeName만 있는 관계로
    //영문이 아닌, native name형태로만 나옴. (차후에 테이블을 구성해주는 쪽으로...)
    CSFCurrencyEnglishName,
    CSFCurrencyNativeName
  };

  Locale();
  Locale(const UString& name);
  Locale(Language language, Country country = AnyCountry);
  Locale(Language language, Script script, Country country);
  Locale(const Locale& rhs);
  Locale& operator = (const Locale& rhs);
  ~Locale();

  Language GetLanguage() const;
  Script GetScript() const;
  Country GetCountry() const;
  UString GetName() const;

  UString GetBcp47Name() const;
  UString GetEnglishLanguageName() const;
  UString GetNativeLanguageName() const;
  UString GetNativeCountryName() const;

  int16 ToInt16(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  uint16 ToUInt16(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  int32 ToInt32(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  uint32 ToUInt32(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  int64 ToInt64(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  uint64 ToUInt64(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  float ToFloat(const UNICHAR* str, int32 len, bool* ok = nullptr) const;
  double ToDouble(const UNICHAR* str, int32 len, bool* ok = nullptr) const;

  //TODO inlines로 빼주는게 좋을듯...
  int16 ToInt16(const UNICHAR* str, bool* ok = nullptr) const;
  uint16 ToUInt16(const UNICHAR* str, bool* ok = nullptr) const;
  int32 ToInt32(const UNICHAR* str, bool* ok = nullptr) const;
  uint32 ToUInt32(const UNICHAR* str, bool* ok = nullptr) const;
  int64 ToInt64(const UNICHAR* str, bool* ok = nullptr) const;
  uint64 ToUInt64(const UNICHAR* str, bool* ok = nullptr) const;
  float ToFloat(const UNICHAR* str, bool* ok = nullptr) const;
  double ToDouble(const UNICHAR* str, bool* ok = nullptr) const;

  int16 ToInt16(const UString& str, bool* ok = nullptr) const;
  uint16 ToUInt16(const UString& str, bool* ok = nullptr) const;
  int32 ToInt32(const UString& str, bool* ok = nullptr) const;
  uint32 ToUInt32(const UString& str, bool* ok = nullptr) const;
  int64 ToInt64(const UString& str, bool* ok = nullptr) const;
  uint64 ToUInt64(const UString& str, bool* ok = nullptr) const;
  float ToFloat(const UString& str, bool* ok = nullptr) const;
  double ToDouble(const UString& str, bool* ok = nullptr) const;

  UString ToString(int64 value) const;
  UString ToString(uint64 value) const;
  UString ToString(int16 value) const;
  UString ToString(uint16 value) const;
  UString ToString(int32 value) const;
  UString ToString(uint32 value) const;
  UString ToString(float value, UNICHAR f = 'f', int32 precision = 6) const; //'f' 'e' 'g' or 'F' 'E' 'G'
  UString ToString(double value, UNICHAR f = 'f', int32 precision = 6) const; //'f' 'e' 'g' or 'F' 'E' 'G'

  UString ToString(const Date& date, const UString& format) const;
  UString ToString(const Date& date, FormatType format = LongFormat) const;
  UString ToString(const Time& time, const UString& format) const;
  UString ToString(const Time& time, FormatType format = LongFormat) const;
  UString ToString(const DateTime& date_time, const UString& format) const;
  UString ToString(const DateTime& date_time, FormatType format = LongFormat) const;

  UString GetTimeFormat(FormatType format = LongFormat) const;
  UString GetDateFormat(FormatType format = LongFormat) const;
  UString GetDateTimeFormat(FormatType format = LongFormat) const;

  Date ToDate(const UString& str, FormatType = LongFormat) const;
  Time ToTime(const UString& str, FormatType = LongFormat) const;
  DateTime ToDateTime(const UString& str, FormatType = LongFormat) const;
  Date ToDate(const UString& str, const UString& format) const;
  Time ToTime(const UString& str, const UString& format) const;
  DateTime ToDateTime(const UString& str, const UString& format) const;

  UNICHAR GetDecimalPoint() const;
  UNICHAR GetGroupSeparator() const;
  UNICHAR GetPercent() const;
  UNICHAR GetZeroDigit() const;
  UNICHAR GetNegativeSign() const;
  UNICHAR GetPositiveSign() const;
  UNICHAR GetExponential() const;

  UString GetMonthName(int32, FormatType format = LongFormat) const;
  UString GetStandaloneMonthName(int32, FormatType format = LongFormat) const;
  UString GetDayName(DayOfWeekType day, FormatType format = LongFormat) const;
  UString GetStandaloneDayName(DayOfWeekType day, FormatType format = LongFormat) const;

  DayOfWeekType GetFirstDayOfWeek() const;
  Array<DayOfWeekType> GetWeekdays() const;

  UString GetAMText() const;
  UString GetPMText() const;

  MeasurementSystem GetMeasurementSystem() const;

  enum LayoutDirection {
    RightToLeft,
    LeftToRight,
  };

  LayoutDirection GetTextDirection() const;

  UString ToUpper(const UString& str) const;
  UString ToLower(const UString& str) const;

  UString GetCurrencySymbol(CurrencySymbolFormat = CSFCurrencySymbol) const;
  UString ToCurrency(int64, const UString& symbol = UString()) const;
  UString ToCurrency(uint64, const UString& symbol = UString()) const;
  UString ToCurrency(int16, const UString& symbol = UString()) const;
  UString ToCurrency(uint16, const UString& symbol = UString()) const;
  UString ToCurrency(int32, const UString& symbol = UString()) const;
  UString ToCurrency(uint32, const UString& symbol = UString()) const;
  UString ToCurrency(float, const UString& symbol = UString(), int32 precision = -1) const;
  UString ToCurrency(double, const UString& symbol = UString(), int32 precision = -1) const;

  bool operator == (const Locale& rhs) const;
  bool operator != (const Locale& rhs) const;

  bool operator <  (const Locale& rhs) const;
  bool operator <= (const Locale& rhs) const;
  bool operator >  (const Locale& rhs) const;
  bool operator >= (const Locale& rhs) const;

  FUN_BASE_API friend uint32 HashOf(const Locale& locale);

  static UString LanguageToString(Language language);
  static UString CountryToString(Country country);
  static UString ScriptToString(Script script);

  static Locale CLocale();
  static Locale System();

  static Array<Locale> MatchingLocales(Language language, Script script, Country country);
  static Array<Locale::Country> CountriesForLanguage(Language language);

  void SetNumberOptions(NumberOptions options);
  NumberOptions GetNumberOptions() const;

  enum QuotationStyle {
    StandardQuotation,
    AlternateQuotation
  };

  UString QuoteString(const UString& str, QuotationStyle style = StandardQuotation) const;

  UString CreateSeparatedList(const Array<UString>& list) const;

  Array<UString> GetUiLanguages() const;

  static Locale GetDefault();
  static void SetDefault(const Locale& locale);

  FUN_BASE_API friend Archive& operator & (Archive& ar, Locale& locale);

 private:
  friend class LocaleImpl;
  Locale(LocaleImpl*);

  ScopedPtr<LocaleImpl> impl_;
};


//
// inlines
//

FUN_ALWAYS_INLINE int16 Locale::ToInt16(const UNICHAR* str, bool* ok) const
{
  return ToInt16(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE uint16 Locale::ToUInt16(const UNICHAR* str, bool* ok) const
{
  return ToUInt16(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE int32 Locale::ToInt32(const UNICHAR* str, bool* ok) const
{
  return ToInt32(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE uint32 Locale::ToUInt32(const UNICHAR* str, bool* ok) const
{
  return ToUInt32(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE int64 Locale::ToInt64(const UNICHAR* str, bool* ok) const
{
  return ToInt64(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE uint64 Locale::ToUInt64(const UNICHAR* str, bool* ok) const
{
  return ToUInt64(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE float Locale::ToFloat(const UNICHAR* str, bool* ok) const
{
  return ToFloat(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE double Locale::ToDouble(const UNICHAR* str, bool* ok) const
{
  return ToDouble(str, CStringTraitsU::Strlen(str), ok);
}

FUN_ALWAYS_INLINE int16 Locale::ToInt16(const UString& str, bool* ok) const
{
  return ToInt16(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE uint16 Locale::ToUInt16(const UString& str, bool* ok) const
{
  return ToUInt16(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE int32 Locale::ToInt32(const UString& str, bool* ok) const
{
  return ToInt32(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE uint32 Locale::ToUInt32(const UString& str, bool* ok) const
{
  return ToUInt32(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE int64 Locale::ToInt64(const UString& str, bool* ok) const
{
  return ToInt64(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE uint64 Locale::ToUInt64(const UString& str, bool* ok) const
{
  return ToUInt64(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE float Locale::ToFloat(const UString& str, bool* ok) const
{
  return ToFloat(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE double Locale::ToDouble(const UString& str, bool* ok) const
{
  return ToDouble(str.ConstData(), str.Len(), ok);
}

FUN_ALWAYS_INLINE UString Locale::ToString(int16 value) const
{
  return ToString((int64)value);
}

FUN_ALWAYS_INLINE UString Locale::ToString(uint16 value) const
{
  return ToString((uint64)value);
}

FUN_ALWAYS_INLINE UString Locale::ToString(int32 value) const
{
  return ToString((int64)value);
}

FUN_ALWAYS_INLINE UString Locale::ToString(uint32 value) const
{
  return ToString((uint64)value);
}

FUN_ALWAYS_INLINE UString Locale::ToString(float value, UNICHAR f, int32 precision) const
{
  return ToString(double(value), f, precision);
}

FUN_ALWAYS_INLINE UString Locale::ToCurrency(int16 value, const UString& symbol) const
{
  return ToCurrency(int64(value), symbol);
}

FUN_ALWAYS_INLINE UString Locale::ToCurrency(uint16 value, const UString& symbol) const
{
  return ToCurrency(uint64(value), symbol);
}

FUN_ALWAYS_INLINE UString Locale::ToCurrency(int32 value, const UString& symbol) const
{
  return ToCurrency(int64(value), symbol);
}

FUN_ALWAYS_INLINE UString Locale::ToCurrency(uint32 value, const UString& symbol) const
{
  return ToCurrency(uint64(value), symbol);
}

FUN_ALWAYS_INLINE UString Locale::ToCurrency(float value, const UString& symbol, int32 precision) const
{
  return ToCurrency(double(value), symbol, precision);
}

FUN_ALWAYS_INLINE Locale Locale::CLocale()
{
  return Locale(C);
}

} // namespace fun
