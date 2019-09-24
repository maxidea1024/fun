#pragma once

#include "fun/http/http.h"
#include "fun/http/response.h"
#include "fun/http/session.h"

#include "fun/base/async/async.h"
#include "fun/base/async/future.h"

#include <functional>
#include <future>

namespace fun {
namespace http {

using AsyncResponse = Future<Response>;

namespace internal {

template <typename T>
void SetOption(Session& session, T&& value) {
  session.SetOption_INTERNAL(FORWARD_VA_ARGS(value));
}

template <typename T, typename... args>
void SetOption(Session& session, T&& value, args... args) {
  SetOption(session, FORWARD_VA_ARGS(value));
  SetOption(session, FORWARD_VA_ARGS(args)...);
}

}  // namespace internal

//
// GET
//

template <typename... args>
Response Get(args... args) {
  Session session;
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Get();
}

template <typename... args>
AsyncResponse GetAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph, [args...]() -> Response {
    return Get(MoveTemp(args)...);
  });
}

template <typename Then, typename... args>
auto GetCallback(Then then, args... args)
    -> Future<decltype(then(Get(MoveTemp(args)...)))> {
  return Async<Response>(
      EAsyncExecution::TaskGraph,
      [then, args...]() -> Response { return then(Get(MoveTemp(args)...)); });
}

//
// POST
//

template <typename... args>
Response Post(args... args) {
  Session session;
  // internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Post();
}

template <typename... args>
AsyncResponse PostAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph, [args...]() -> Response {
    return Post(MoveTemp(args)...);
  });
}

template <typename Then, typename... args>
auto PostCallback(Then then, args... args)
    -> Future<decltype(then(Post(MoveTemp(args)...)))> {
  return Async<Response>(
      EAsyncExecution::TaskGraph,
      [then, args...]() -> Response { return then(Post(MoveTemp(args)...)); });
}

//
// PUT
//

template <typename... args>
Response Put(args... args) {
  Session session;
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Put();
}

template <typename... args>
AsyncResponse PutAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph,
                         [args...]() { return Put(MoveTemp(args)...); });
}

template <typename Then, typename... args>
auto PutCallback(Then then, args... args)
    -> Future<decltype(then(Put(MoveTemp(args)...)))> {
  return Async<Response>(EAsyncExecution::TaskGraph, [then, args...]() {
    return then(Put(MoveTemp(args)...));
  });
}

//
// HEAD
//

template <typename... args>
Response Head(args... args) {
  Session session;
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Head();
}

template <typename... args>
AsyncResponse HeadAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph, [args...]() -> Response {
    return Head(MoveTemp(args)...);
  });
}

template <typename Then, typename... args>
auto HeadCallback(Then then, args... args)
    -> Future<decltype(then(Head(MoveTemp(args)...)))> {
  return Async<Response>(
      EAsyncExecution::TaskGraph,
      [then, args...]() -> Response { return then(Head(MoveTemp(args)...)); });
}

//
// DELETE
//

template <typename... args>
Response Delete(args... args) {
  Session session;
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Delete();
}

template <typename... args>
AsyncResponse DeleteAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph, [args...]() -> Response {
    return Delete(MoveTemp(args)...);
  });
}

template <typename Then, typename... args>
auto DeleteCallback(Then then, args... args)
    -> Future<decltype(then(Delete(MoveTemp(args)...)))> {
  return Async<Response>(EAsyncExecution::TaskGraph,
                         [then, args...]() -> Response {
                           return then(Delete(MoveTemp(args)...));
                         });
}

//
// OPTIONS
//

template <typename... args>
Response Options(args... args) {
  Session Session;
  internal::SetOption(Session, FORWARD_VA_ARGS(args)...);
  return Session.Options();
}

template <typename... args>
AsyncResponse OptionsAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph,
                         [args...]() { return Options(MoveTemp(args)...); });
}

template <typename Then, typename... args>
auto OptionsCallback(Then then, args... args)
    -> Future<decltype(then(Options(MoveTemp(args)...)))> {
  return Async<Response>(EAsyncExecution::TaskGraph, [then, args...]() {
    return then(Options(MoveTemp(args)...));
  });
}

//
// PATCH
//

template <typename... args>
Response Patch(args... args) {
  Session session;
  internal::SetOption(session, FORWARD_VA_ARGS(args)...);
  return session.Patch();
}

template <typename... args>
AsyncResponse PatchAsync(args... args) {
  return Async<Response>(EAsyncExecution::TaskGraph,
                         [args...]() { return Patch(MoveTemp(args)...); });
}

template <typename Then, typename... args>
auto PatchCallback(Then then, args... args)
    -> Future<decltype(then(Patch(MoveTemp(args)...)))> {
  return Async<Response>(EAsyncExecution::TaskGraph,
                         [then, args...](Then then, args... args) {
                           return then(Patch(MoveTemp(args)...));
                         });
}

}  // namespace http
}  // namespace fun
