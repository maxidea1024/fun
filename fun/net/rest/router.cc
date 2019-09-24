

Request::Request(const http::Request& request,
                  Array<TypedParam>&& args,
                  Array<TypedParam>&& splats)
  : http::Request(request)
  , params_(MoveTemp(args))
  , splats_(MoveTemp(splats_)
{
  
}


bool Request::HasParam(const String& name) const
{
  
}


TypedParam Request::GetParam(const String& name) const
{
  
}


TypedParam Request::GetSplatAt(size_t index) const
{
  
}


Array<TypedParam> Request::GetSplats() const
{
  return splats_;
}





Route::Fragment::Match(const String& raw) const
{
  
}


bool Route::Fragment::Match(const Fragment& other) const
{
  
}


void Route::Fragment::Init(String value)
{
  
}


void Route::Fragment::CheckInvariant() const
{
  
}


Array<Route::Fragment> Route::Fragment::FromUrl(const String& url)
{
  
}


bool Route::Fragment::IsParameter() const
{
  
}


bool Route::Fragment::IsOptional() const
{
  
}


bool Route::Fragment::IsSplat() const
{
  
}


Tuple<bool,Array<TypedParam>,Array<TypedParam>> Route::Match(const http::Request& req) const
{
  
}


Tuple<bool,Array<TypedParam>,Array<TypedParam>> Route::Match(const String& req) const
{
  
}



namespace internal {


RouterHandler::RouterHandler(const Router& router)
  : router(router)
{
  
}


void RouterHandler::OnRequest(const http::Request& req, http::ResponseWriter& response)
{
  
}


} // namespace internal



Router Router::FromDescription(const rest::Description& desc)
{
  
}


SharedPtr<internal::RouterHandler> Router::GetHandler() const
{
  
}


void Router::InitFromDescription(const rest::Description& desc)
{
  
}



void Router::Get(String resource, Route::Handler handler)
{
  
}


void Router::Post(String resource, Route::Handler handler)
{
  
}


void Router::Put(String resource, Route::Handler handler)
{
  
}


void Router::Patch(String resource, Route::Handler handler)
{
  
}


void Router::Delete(String resource, Route::Handler handler)
{
  
}


void Router::Options(String resource, Route::Handler handler)
{
  
}


void Router::AddCustomHandler(Route::Handler handler)
{
  
}


void Router::AddNotFoundHandler(Route::Handler handler)
{
  
}


void Router::InvokeNotFoundHandler(const http::Request& req, http::ResponseWriter& response) const
{
  
}


Router::Status Router::Route(const http::Request& req, http::ResponseWriter& response)
{
  
}


void Router::AddRoute(http::Method method, String resource, Route::Handler handler)
{
  
}



namesace Routes {


void Get(Router& router, String resource, Route::Handler handler)
{
  
}


void Post(Router& router, String resource, Route::Handler handler)
{
  
}


void Put(Router& router, String resource, Route::Handler handler)
{
  
}


void Patch(Router& router, String resource, Route::Handler handler)
{
  
}


void Delete(Router& router, String resource, Route::Handler handler)
{
  
}


void Options(Router& router, String resource, Route::Handler handler)
{
  
}


void NotFound(Router& router, String resource, Route::Handler handler)
{
  router.AddNotFoundHandler(MoveTemp(handler));
}



} // namespace routers


} // namespace rest
} // namespace fun
