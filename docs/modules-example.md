# Modules example

Example called `my_app` which shows a simple user session manager and authentication server interacting to create user tokens (such as JWTs). It is made of two nodes (`Session` and `AuthService`) in a cluster, which express their dependencies on each other via connections in the cluster. The connections are named by the trait that satisfies the dependency.

The nodes also have two dependencies on data types:
1. `Session` derives its `PassHash` type from the one provided at the root of the graph via `Context::Root`.
2. `Session` derives its `Token` type from the one provided by `AuthService` via `arc::ResolveTypes` static trait resolution.

<details>
<summary>:warning: NOTE: the data type dependencies are simply showing off features! Don't resolve data types inside nodes just because you can!</summary>

> It is generally recommended for simplicity that well-known data types used by multiple nodes are defined _independently_ of and _externally_ to nodes. Humans tend to find types much more easily through a well organised file structure, rather than traversing clusters of interdependent nodes. If a data type does need to be configurable using `Context::Root` or `ResolveTypes`, the concrete type itself still should be independently defined in a well-named class that is easy to find. It can then be simply aliased in the custom root or the node's trait types.
>
> Order of preference for dependencies on data types:
> 1. Type defined externally and used directly by nodes
>    - No type resolution keeps things explicit
>    - Easier to define non-template member functions, which can easily be split into a separate .cpp for faster compilation
> 2. Type defined externally and aliased in a custom graph root
>    - Type alias is easy to find without walking any nodes of the graph
> 3. Type defined externally and aliased in node trait types _if number of affected nodes and traits is small_
>    - Type alias is not too distant from the current node
> 4. Type defined internally to a mock node for testing purposes and aliased in the trait types
>    - Mocks are defined in the same source file as the tests

</details>

## File structure
- [my/CMakeLists.txt](#cmakeliststxt)
- [my/traits.ixx.arc](#traitsixxarc): `my::trait::AuthService`, `my::trait::TokenStore`, `my::trait::SessionManager`: traits for nodes to implement
- [my/cluster.ixx.arc](#clusterixxarc): `my::Cluster`: A cluster of interconnected nodes implementing the traits
- [my/token.ixx](#tokenixx): `my::Token` data type stores the proof of identity for a user, issued by the AuthService with an expiry
- [my/pass_hash.ixx](#pass_hashixx): `my::PassHash` data type stores the hash of passwords
- [my/auth_service.ixx](#auth_serviceixx): `my::AuthService` node implements the trait `my::trait::AuthService`
- [my/sessions.ixx](#sessionsixx): `my::Sessions` node implements the traits `my::trait::TokenStore` and `my::trait::SessionManager`
- [my/logger.ixx](#loggerixx): `my::Logger` node implements the trait `my::trait::Logger`
- [my/main.cpp](#maincpp): Constructs and uses the full graph of nodes which satisfies all requirements of the nodes within `my::Cluster`
- [Unit tests](#unittests): Unit test `my::trait::SessionManager` trait of `my::Sessions` using a `AuthServiceTestDouble` test double, and also the `arc::test::Mock` mocking node

## CMakeLists.txt
```CMake
add_executable(my_app main.cpp)

target_link_libraries(my_app PUBLIC arc::module)

# Add module interface units and implementation units
target_sources(my_app
    PUBLIC FILE_SET CXX_MODULES FILES
        token.ixx
        pass_hash.ixx
        auth_service.ixx
        sessions.ixx
        db.ixx # not shown
        task.ixx # not shown
    PRIVATE
        auth_service.cpp # not shown (implements AuthService::dbValidPass etc)
        token.cpp # not shown
        pass_hash.cpp # not shown
)

# Adds traits.ixx (from traits.ixx.arc) and cluster.ixx (from cluster.ixx.arc)
# by searching recursively for *.ixx.arc in the current directory
target_generate_arc_modules(my_app)
```

## traits.ixx.arc
`my::trait::AuthService`, `my::trait::TokenStore`, `my::trait::SessionManager`: traits for nodes to implement. See full trait syntax [documentation](trait-syntax.md).
```cpp
export module my.traits;

import std;
import my.task; // not shown in example

namespace my::trait {

trait AuthService
{
    // Ensures Token type can be resolved by clients of this trait
    type Token

    // Check user/pass against database, and return if combo is valid
    logIn(std::string_view user, std::string_view pass) -> Task<bool>

    // Check user/oldPass against database, and return if password was changed
    changePass(std::string_view user, std::string_view oldPass, std::string_view newPass) -> Task<bool>
}

trait TokenStore [Types, Root]
{
    // Ensures PassHash is defined at the root of the graph when using this trait
    root PassHash
    type Token

    // Store latest token generated for a user
    store(std::string_view user, Root::PassHash passHash, Types::Token token)

    // Delete any token stored against this user
    revoke(std::string_view user)
}

trait SessionManager [Types]
{
    type Token

    // Has a token been created for user that has not expired
    hasToken(std::string_view user) const -> bool

    // Get token for user using the password if necessary
    getToken(std::string_view user, std::string_view pass) -> Task<std::optional<typename Types::Token>>
}

trait Logger
{
    // Takes any std::format compatible arguments
    log(...)
}

}
```

## cluster.ixx.arc
`my::Cluster`: A cluster of interconnected nodes implementing the traits. See full cluster syntax [documentation](cluster-syntax.md).
```cpp
export module my.cluster;

import my.sessions;
import my.auth_service;
import my.traits;

namespace my {

cluster Cluster
{
    sessions = Sessions
    authService = AuthService

    [trait::TokenStore <-> trait::AuthService]
    sessions <-> authService

    // Above is equivalent to:
    // [trait::TokenStore]  sessions <-- authService
    // [trait::AuthService] sessions --> authService

    // Explicitly redirect sessions to the global logger so it can be resolved like a normal dependency (optional)
    [trait::Logger]
    sessions --> @global
}

}
```
## token.ixx
`my::Token` data type stores the proof of identity for a user, issued by the AuthService with an expiry
```cpp
export module my.token;

namespace my {
    export struct Token
    {
        Token(std::string_view tokenSecret, std::string_view user, std::chrono::seconds expiry);
        bool expired() const;
    private:
        // ... some data fields ...
    };
}
```
## pass_hash.ixx
`my::PassHash` data type stores the hash of passwords
```cpp
export module my.pass_hash;

namespace my {
    export struct PassHash
    {
        explicit PassHash(std::string_view s);
        friend constexpr bool sameHash(PassHash const& self, std::string_view pass);
    private:
        // ... some data fields ...
    };
}
```
## auth_service.ixx
`my::AuthService` node implements the trait `my::trait::AuthService`. See node [documentation](node-structure.md).
```cpp
export module my.auth_service;

import my.token;
import my.traits;
import my.db; // not shown in example
import my.task; // not shown in example

import arc;
import std;

namespace my {

// Note: non-template functions in non-template class can be easily implemented
// in separate .cpp files for faster compilation.
export struct AuthService : arc::Node
{
    // `AuthService` uses `trait::TokenStore` via `getNode`, and `trait::Logger` via `getGlobal`
    // Both are checked when compiling the graph that hosts `AuthService`
    using Depends = arc::Depends<trait::TokenStore, arc::Global<trait::Logger>>;

    // `AuthService` implements the trait `my::trait::AuthService`
    using Traits = arc::Traits<AuthService, trait::AuthService>;
    // shorthand for:
    //      arc::Traits<AuthService
    //          , trait::AuthService(AuthService, AuthService::Types)
    //      >

    struct Types
    {
        // exposes `Token` type to client nodes, as is required by `trait::AuthService`
        using Token = my::Token;
    };

    // Trait methods are to be implemented with the signature: impl(<Trait>::<method>, args...)
    template<class Self>
    Task<bool> impl(
        this Self& self, // deducing-this deduces the node's context in the graph
        trait::AuthService::logIn,
        // take user+pass as owning strings as this is a coroutine (still meets trait requirement)
        std::string user,
        std::string pass)
    {
        self.dbQueryCount++;
        bool const success = co_await self.dbValidPass(user, pass);
        if (success)
        {
            // Resolves to `my::PassHash` in this example
            using PassHash = arc::ContextOf<Self>::Root::PassHash;
            // Since the type and memory offset of `Sessions` with respect to `AuthService` is known
            // statically via the context of `Self`, this entire call can/will be inlined by the compiler:
            self.getNode(trait::tokenStore).store(user, PassHash(pass), Token(self.tokenSecret, user, self.expiry));
            // In this example project, this is effectively equivalent to:
            // <my::Cluster>.sessions.asTrait(trait::tokenStore).store(user, PassHash(...), Token(...));
            // which directly calls
            // <my::Cluster>.sessions.impl(trait::TokenStore::store{}, user, PassHash(...), Token(...));
        }
        // Global dependencies resolved via `getGlobal` can be omitted from the graph
        self.getGlobal(trait::logger).log("User {} logged in successfully: {}", user, success);
        co_return success;
    }

    Task<bool> impl(
        this auto& self,
        trait::AuthService::changePass,
        std::string user,
        std::string oldPass,
        std::string newPass)
    {
        self.dbQueryCount++;
        bool const success = co_await self.dbChangePass(user, oldPass, newPass);
        if (success)
            self.getNode(trait::tokenStore).revoke(user);
        co_return success;
    }

    // These are implemented in a separate file: auth_service.cpp (not shown)
    // Queries database asynchronously and returns `true` in calling thread if the pass is valid.
    Task<bool> dbValidPass(std::string_view user, std::string_view pass) const;
    Task<bool> dbChangePass(std::string_view user, std::string_view oldPass, std::string_view newPass) const;

    AuthService(std::string tokenSecret, std::chrono::seconds expiry)
        : tokenSecret(std::move(tokenSecret))
        , expiry(expiry)
        , db(makeDbHandle(...))
    {}

    std::size_t dbQueryCount = 0;
    std::string tokenSecret;
    std::chrono::seconds expiry;
    DatabaseHandle db;
};

}
```
## sessions.ixx
`my::Sessions` node implements the traits `my::trait::TokenStore` and `my::trait::SessionManager`. See node [documentation](node-structure.md).
```cpp
export module my.sessions;

import my.traits;

import arc;
import std;

namespace my {

export struct Sessions
{
    // If implementing a node with data type dependencies in its state, a nested
    // Node<Context> template class is needed to query the types in the graph
    template<class Context>
    struct Node : arc::Node
    {
        // Since `sessions` is explicitly redirected to the global node `@global` in the graph,
        // we can simply depend on `trait::Logger` instead of `arc::Global<trait::Logger>`
        using Depends = arc::Depends<trait::AuthService, trait::Logger>;

        using Traits = arc::Traits<Node, trait::TokenStore, trait::SessionManager>;
        // shorthand for:
        //      arc::Traits<Node
        //          , trait::TokenStore(Node, Node::Types)
        //          , trait::SessionManager(Node, Node::Types)
        //      >

        struct Types
        {
            // Exposes Token type to satisfy `trait::TokenStore` and `trait::SessionManager`
            // by deferring to the type provided by `trait::AuthService`
            using Token = arc::ResolveTypes<Node, trait::AuthService>::Token;
            // Resolves to `my::AuthService::Types::Token` in this example
        };

        using Token = Types::Token;

        // Resolves to `my::PassHash` in this example
        using PassHash = Context::Root::PassHash;

        void impl(trait::TokenStore::store, std::string_view user, PassHash passHash, Token token)
        {
            // `trait::Logger` dependency is explicitly resolved in the graph,
            // so we can call it directly with `getNode` instead of `getGlobal`
            getNode(trait::logger).log("Storing token for user: {}", user);
            auto const [it, inserted] = userDetailsMap.try_emplace(user, std::move(passHash), std::move(token));
            if (not inserted)
            {
                it->second.passHash = std::move(passHash);
                it->second.token = std::move(token);
            }
        }

        void impl(trait::TokenStore::revoke, std::string_view user)
        {
            getNode(trait::logger).log("Revoking token for user: {}", user);
            userDetailsMap.erase(user);
        }

        bool impl(trait::SessionManager::hasToken, std::string_view user) const
        {
            if (auto const userDetails = getUserDetails(user))
                if (not userDetails->token.expired())
                    return true;
            return false;
        }

        Task<std::optional<Token>> impl(
            trait::SessionManager::getToken,
            std::string_view user,
            std::string_view pass)
        {
            // To avoid db query: return cached token if one exists that has not expired
            if (auto const userDetails = getUserDetails(user))
                if (userDetails->validWithPass(pass))
                    co_return userDetails->token;

            // Else, try to log in and return token if successful
            // Note: `getNode` can be called directly, since `Context` is immediately available
            if (co_await getNode(trait::authService).logIn(user, pass))
                co_return getUserDetails(user)->token;

            co_return std::nullopt;
        }

        auto const* getUserDetails(std::string_view user) const
        {
            auto const tokenIt = userDetailsMap.find(user);
            return tokenIt != userDetailsMap.end()
                ? std::addressof(tokenIt->second)
                : nullptr;
        }

        Node() = default;

        struct UserDetails
        {
            PassHash passHash; // avoid storing passwords in plaintext
            Token token;

            bool validWithPass(std::string_view pass) const
            {
                return not token.expired() and sameHash(passHash, pass);
            }
        };

        std::unordered_map<std::string, UserDetails> userDetailsMap;
    };
};

}
```

## logger.ixx
`my::Logger` node implements the trait `my::trait::Logger` (which will be the global node)
```cpp
export module my.logger;

import my.traits;
import arc;
import std;

namespace my {

export struct Logger : arc::Node
{
    using Traits = arc::Traits<Logger, trait::Logger>;

    template<class... Args>
    void impl(trait::Logger::log, Args&&... args) const
    {
        std::println(std::forward<Args>(args)...);
    }
};

}
```
## main.cpp
Constructs the full graph of nodes which satisfies all requirements of the nodes within `my::Cluster`
```cpp
import my.cluster;
import my.logger;
import my.pass_hash;
import my.traits;
import arc;

using namespace my;

int main()
{
    struct Root
    {
        using PassHash = my::PassHash;
    };

    arc::GraphWithGlobal<my::Cluster, my::Logger, Root> graph{
        .global{}, // my::Logger is the global node
        .main{ // my::Cluster is the main cluster
            .authService{"super token secret", std::chrono::seconds{600}},
        },
    };
    // `arc::Graph<my::Cluster, Root>` is an alias to `my::Cluster<arc::RootContext<Root>>`, which is roughly:
    //  struct my::PseudoGeneratedCluster
    //  {
    //      struct SessionsContext { ... };
    //      struct AuthServiceContext { ... };
    //
    //      using SessionsNode    = Sessions::template Node<SessionsContext>;
    //      using AuthServiceNode = arc::WrapNode<AuthService>::template Node<AuthServiceContext>;
    //
    //      SessionsNode    sessions{};
    //      AuthServiceNode authService{};
    //
    //      void visit(auto const& visitor)
    //      {
    //          sessions.visit(visitor);
    //          authService.visit(visitor);
    //      }
    //  };

    auto sessionManager = graph.main.sessions.asTrait(trait::sessionManager);
    assert(not sessionManager.hasToken("user1")); // user1 not logged in yet
    // same as sessionManager.impl(trait::SessionManager::hasToken{}, "user1")
    // which directly calls graph.sessions.impl(trait::SessionManager::hasToken{}, "user1")

    // Note, the following do not compile (inaccessible) as they are not part of `trait::SessionManager`:
    // sessionManager.userDetails;
    // sessionManager.getUserDetails("user1");
    // sessionManager.getNode(trait::authService);
    // sessionManager.impl(trait::TokenStore::store, "user1", Token{});
    // sessionManager.store("user1", Token{});
    // This ensures that the details of a node cannot leak from a trait, keeping the interface robust,
    // and absolves nodes of the responsibility to hide away their state and implementation details,
    // since other nodes in the graph are only able to access their traits via getNode(<trait>).

    auto authService = graph.main.authService.asTrait(trait::authService);
    auto user1LoginTask = authService.logIn("user1", "user1Pass");
    if (user1LoginTask.value()) // synchronously waits for task to complete (as we are in main)
    {
        // assuming the token hasn't expired yet
        assert(sessionManager.hasToken("user1"));
        assert(sessionManager.getToken("user1", "user1Pass").value().has_value());
        // change password should be successful
        assert(authService.changePass("user1", "user1Pass", "newUser1Pass").value());
        // token has been revoked
        assert(not sessionManager.hasToken("user1"));
        // can get token with new password
        assert(sessionManager.getToken("user1", "newUser1Pass").value().has_value());
    }
}
```

## <a name="unittests"></a>Unit testing trait::SessionManager trait of my::Sessions
```cpp
#include <doctest/doctest.h>

import my.sessions;
import my.traits;
import arc;
import std;

namespace my::tests::session_manager {

struct MockRoot
{
    struct PassHash
    {
        std::string pass;
        friend constexpr bool sameHash(PassHash const& self, std::string_view pass)
        {
            return self.pass == pass;
        }
    };
};

struct MockTypes
{
    struct Token
    {
        std::size_t id;
        std::size_t expiry;
        std::size_t const* time;

        bool expired() const { return *time < expiry; }
        auto operator<=>(Token const&) const = default;
    };
};

// TestOnlyNode is a node that can only be used inside a test context
struct MockLogger : arc::TestOnlyNode
{
    using Traits = arc::Traits<MockLogger, trait::Logger>;

    void impl(trait::Logger::log, auto&&...) const
    {
        // no-op logger
    }
};

inline constexpr std::string_view theUser = "user";
inline constexpr std::string_view thePass = "pass";
inline constexpr std::string_view wrongPass = "wrongPass";

static void testSessionManager(
    arc::IsTraitViewOf<trait::SessionManager> auto sessionManager,
    std::invocable<std::size_t> auto increaseTime)
{
    using Token = MockTypes::Token;

    CHECK(not sessionManager.hasToken(theUser));

    // Wrong password fails to get a token
    std::optional<Token> token = sessionManager.getToken(theUser, wrongPass).value();
    CHECK(not token.has_value())
    // Correct password gets a token
    token = sessionManager.getToken(theUser, thePass).value();
    CHECK(token.has_value());

    // Check token expiry behaviour
    increaseTime(1);
    CHECK(not token->expired());
    CHECK(sessionManager.hasToken(theUser));
    // When token in the store has not expired, a new token is not issued
    auto token2 = sessionManager.getToken(theUser, thePass).value();
    CHECK(token == token2);

    increaseTime(1);
    CHECK(token->expired());
    CHECK(token2->expired());
    CHECK(not sessionManager.hasToken(theUser));
    // New token is issued when old one has expired
    token2 = sessionManager.getToken(theUser, thePass).value();
    CHECK(token2.has_value());
    CHECK(not token2->expired());
    CHECK(token2 != token);
}

// Test doubles are reusable across tests in the whole project
struct AuthServiceTestDouble : arc::TestOnlyNode
{
    using Traits = arc::Traits<AuthServiceTestDouble, my::trait::AuthService>;

    using Types = MockTypes;

    // NOTE: In a test context, is it not required for all of trait::AuthService to be implemented,
    // only those methods which are invoked from the node being tested
    my::Task<bool> impl(this auto& self, trait::AuthService::logIn, std::string_view user, std::string_view pass)
    {
        if (userPass[user] == pass)
        {
            // Each generated token has unique id
            Token token{.id = self.tokenId++, .expiry = self.time + 2, .time = &self.time};
            PassHash passHash{pass};
            self.getNode(trait::tokenStore).store(user, passHash, token);
            co_return true;
        }
        co_return false;
    };

    void addUser(std::string const& user, std::string const& pass)
    {
        userPasses.insert_or_assign(user, pass);
    }

    void passTime(std::size_t increase) { time += increase; }

private:
    std::size_t time = 0;
    std::size_t tokenId = 0;
    std::map<std::string, std::string> userPasses;
};

TEST_SUITE("my::Sessions as trait::SessionManager")
{
    TEST_CASE("Using AuthServiceTestDouble")
    {
        arc::test::Graph<my::Sessions, arc::Combine<MockLogger, AuthServiceTestDouble>, MockRoot> graph;
        // With `arc::test::Graph`, any and all `getNode` calls in `my::Sessions` resolve to
        // `arc::Combine<MockLogger, AuthServiceTestDouble>`, which selects the node which implements the required trait

        graph.mocks->addUser(theUser, thePass);
        auto sessionManager = graph.node.asTrait(trait::sessionManager);
        auto const incrementTime = [&](std::size_t inc) { graph.mocks->passTime(inc); };
        testSessionManager(sessionManager, incrementTime);
    }

    TEST_CASE("Using arc::test::Mock")
    {
        using Mocks = arc::test::Mock<MockTypes>;
        arc::test::Graph<my::Sessions, Mocks, MockRoot> graph;
        // With `arc::test::Graph`, any and all `getNode` calls in `my::Sessions` resolve to `Mocks`

        std::size_t time = 0;
        std::size_t tokenId = 0;

        // Mock all trait dependencies that are used by my::Sessions
        // NOTE: Not all of `trait::AuthService` needs to be defined in the mock,
        // since `AuthService::changePass` is not called from `my::Sessions`
        // NOTE: `trait::Logger::log` does not need to be explicitly mocked,
        // the default implementation for any missing method is a no-op
        graph.mocks->define(
            [&](trait::AuthService::logIn, std::string user, std::string pass) -> my::Task<bool>
            {
                if (user == theUser and pass == thePass)
                {
                    // Each generated token has unique id
                    Token token{.id = tokenId++, .expiry = time + 2, .time = &time};
                    PassHash passHash{pass};
                    graph.node.asTrait(trait::tokenStore).store(theUser, passHash, token);
                    co_return true;
                }
                co_return false;
            });

        auto sessionManager = graph.node.asTrait(trait::sessionManager);
        auto const incrementTime = [&](std::size_t inc) { time += inc; };
        testSessionManager(sessionManager, incrementTime);
    }
}

}
```
