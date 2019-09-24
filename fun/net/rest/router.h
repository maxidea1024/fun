#include "fun/net/net.h"
#include "fun/net/http/http_defs.h"
#include "fun/base/flags.h"

namespace fun {
namespace rest {

class TypedParam {
public:
  TypedParam(const String& name, const String& value)
    : name_(name), value_(value) {}

  template <typename T>
  T As() const {
    //TODO
  }

  const String& GetName() const {
    return name_;
  }

private:
  String name_;
  String value_;
}

class Request;

class Route {
public:
  enum class Result {
    Ok,
    Failure
  };

  typedef Function<Result (const Request&, http::ResponseWriter)> Handler;

  Route(String resource, http::Method method, Handler handler)
    : resource_(MoveTemp(resource)),
      handler_(MoveTemp(handler)),
      fragments_(Fragment::FromUrl(resource_)) {
    FUN_UNUSED(method); //??
  }

  Tuple<bool,Array<TypedParam>,Array<TypedParam>> Match(const http::Request& req) const;
  Tuple<bool,Array<TypedParam>,Array<TypedParam>> Match(const String& req) const;

  template <typename... args>
  void InvokeHandler(args&& ...args) const {
    handler_(Forward<args>(args)...);
  }

private:
  struct Fragment {
    explicit Fragment(String value);

    bool Match(const String& raw) const;
    bool Match(const Fragment& other) const;

    bool IsParameter() const;
    bool IsSplat() const;
    bool IsOptional() const;

    String GetValue() const {
      return value_;
    }

    static Array<Fragment> FromUrl(const String& url);

   private:
    enum class Flag {
      None = 0,
      Fixed = 1,
      Parameter = Fixed << 1,
      Optional = Parameter << 1,
      Splat = Optional << 1,
    };

    void Init(String value);
    void CheckInvariant() const;

    Flags<Flag> flags_;
    String value_;
  };

  String resource_;
  Handler handler_;
  Array<Fragment> fragments_;
};

namespace internal {

class RouteHandler;

}

class Router {
 public:
  enum class Status { Match, NotFound };

  static Router fromDescription(const Rest::Description& desc);

  std::shared_ptr<Private::RouterHandler>
  handler() const;

  void initFromDescription(const Rest::Description& desc);

  void get(std::string resource, Route::Handler handler);
  void post(std::string resource, Route::Handler handler);
  void put(std::string resource, Route::Handler handler);
  void patch(std::string resource, Route::Handler handler);
  void del(std::string resource, Route::Handler handler);
  void options(std::string resource, Route::Handler handler);

  void addCustomHandler(Route::Handler handler);

  void addNotFoundHandler(Route::Handler handler);
  inline bool hasNotFoundHandler() { return notFoundHandler != nullptr; }
  void invokeNotFoundHandler(const Http::Request &req, Http::ResponseWriter resp) const;

  Status route(const Http::Request& request, Http::ResponseWriter response);

 private:
  void addRoute(Http::Method method, std::string resource, Route::Handler handler);
  std::unordered_map<Http::Method, std::vector<Route>> routes;

  std::vector<Route::Handler> customHandlers;

  Route::Handler notFoundHandler;
};


namespace internal {

class RouterHandler : public Http::Handler {
 public:
  RouterHandler(const Rest::Router& router);

  void onRequest(
          const Http::Request& req,
          Http::ResponseWriter response);

 private:
  std::shared_ptr<Tcp::Handler> clone() const {
      return std::make_shared<RouterHandler>(*this);
  }

  Rest::Router router;
};

}

class Request : public Http::Request {
 public:
  friend class Router;

  bool hasParam(const std::string& name) const;
  TypedParam param(const std::string& name) const;

  TypedParam splatAt(size_t index) const;
  std::vector<TypedParam> splat() const;

 private:
  explicit Request(
          const Http::Request& request,
          std::vector<TypedParam>&& args,
          std::vector<TypedParam>&& splats);

  std::vector<TypedParam> params_;
  std::vector<TypedParam> splats_;
};

namespace routes {

    void Get(Router& router, std::string resource, Route::Handler handler);
    void Post(Router& router, std::string resource, Route::Handler handler);
    void Put(Router& router, std::string resource, Route::Handler handler);
    void Patch(Router& router, std::string resource, Route::Handler handler);
    void Delete(Router& router, std::string resource, Route::Handler handler);
    void Options(Router& router, std::string resource, Route::Handler handler);

    void NotFound(Router& router, Route::Handler handler);

    namespace details {
        template <class... args>
        struct TypeList
        {
            template <size_t N>
            struct At {
                static_assert(N < sizeof...(args), "Invalid index");
                typedef typename std::tuple_element<N, std::tuple<args...>>::type Type;
            };
        };

        template <typename... args>
        void static_checks() {
            static_assert(sizeof...(args) == 2, "Function should take 2 parameters");
//            typedef details::TypeList<args...> Arguments;
            // Disabled now as it
            // 1/ does not compile
            // 2/ might not be relevant
#if 0
            static_assert(std::is_same<Arguments::At<0>::Type, const Rest::Request&>::value, "First argument should be a const Rest::Request&");
            static_assert(std::is_same<typename Arguments::At<0>::Type, Http::Response>::value, "Second argument should be a Http::Response");
#endif
        }
    }


    template <typename Result, typename Cls, typename... args, typename Obj>
    Route::Handler bind(Result (Cls::*func)(args...), Obj obj) {
        details::static_checks<args...>();

        #define CALL_MEMBER_FN(obj, pmf)  ((obj)->*(pmf))

        return [=](const Rest::Request& request, Http::ResponseWriter response) {
            CALL_MEMBER_FN(obj, func)(request, std::move(response));

            return Route::Result::Ok;
        };

        #undef CALL_MEMBER_FN
    }

    template <typename Result, typename... args>
    Route::Handler bind(Result (*func)(args...))
    {
        details::static_checks<args...>();

        return [=](const Rest::Request& request, Http::ResponseWriter response) {
            func(request, std::move(response));

            return Route::Result::Ok;
        };
    }

} // namespace routes

} // namespace rest
} // namespace fun
