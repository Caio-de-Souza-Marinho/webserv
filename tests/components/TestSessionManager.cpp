#include "../../include/SessionManager.hpp"
#include "TestUtils.hpp"

#include <string>
#include <set>

// ---------------------------------------------------------------------------
// resolveSession – empty cookie header creates a new session
// ---------------------------------------------------------------------------

static void testResolveEmptyCookieCreatesSession()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("", isNew);

	ASSERT_TRUE(!id.empty());
	ASSERT_TRUE(isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – unknown session_id in cookie creates a new session
// ---------------------------------------------------------------------------

static void testResolveUnknownCookieCreatesSession()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("session_id=does_not_exist", isNew);

	ASSERT_TRUE(!id.empty());
	ASSERT_TRUE(isNew);
	ASSERT_TRUE(id != "does_not_exist");
}

// ---------------------------------------------------------------------------
// resolveSession – known session_id returns same id, isNew = false
// ---------------------------------------------------------------------------

static void testResolveKnownCookieReturnsSameId()
{
	SessionManager	sm;
	bool		isNew;

	std::string first = sm.resolveSession("", isNew);
	ASSERT_TRUE(isNew);

	std::string cookie = "session_id=" + first;
	std::string second = sm.resolveSession(cookie, isNew);

	ASSERT_EQ(first, second);
	ASSERT_TRUE(!isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – each fresh call produces a unique id
// ---------------------------------------------------------------------------

static void testResolveProducesUniqueIds()
{
	SessionManager		sm;
	bool			isNew;
	std::set<std::string>	ids;

	for (int i = 0; i < 20; ++i)
	{
		std::string id = sm.resolveSession("", isNew);
		ASSERT_TRUE(ids.find(id) == ids.end()); // must not have been seen before
		ids.insert(id);
	}
}

// ---------------------------------------------------------------------------
// resolveSession – cookie with multiple fields, session_id first
// ---------------------------------------------------------------------------

static void testResolveMultipleCookiesSessionFirst()
{
	SessionManager	sm;
	bool		isNew;

	std::string first = sm.resolveSession("", isNew);

	std::string cookie = "session_id=" + first + "; theme=dark; lang=en";
	std::string second = sm.resolveSession(cookie, isNew);

	ASSERT_EQ(first, second);
	ASSERT_TRUE(!isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – cookie with multiple fields, session_id last
// ---------------------------------------------------------------------------

static void testResolveMultipleCookiesSessionLast()
{
	SessionManager	sm;
	bool		isNew;

	std::string first = sm.resolveSession("", isNew);

	std::string cookie = "theme=dark; lang=en; session_id=" + first;
	std::string second = sm.resolveSession(cookie, isNew);

	ASSERT_EQ(first, second);
	ASSERT_TRUE(!isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – cookie with multiple fields, session_id in the middle
// ---------------------------------------------------------------------------

static void testResolveMultipleCookiesSessionMiddle()
{
	SessionManager	sm;
	bool		isNew;

	std::string first = sm.resolveSession("", isNew);

	std::string cookie = "theme=dark; session_id=" + first + "; lang=en";
	std::string second = sm.resolveSession(cookie, isNew);

	ASSERT_EQ(first, second);
	ASSERT_TRUE(!isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – cookie header with no session_id key at all
// ---------------------------------------------------------------------------

static void testResolveNoSessionIdKey()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("theme=dark; lang=en", isNew);

	ASSERT_TRUE(!id.empty());
	ASSERT_TRUE(isNew);
}

// ---------------------------------------------------------------------------
// resolveSession – generated id starts with "sess_"
// ---------------------------------------------------------------------------

static void testGeneratedIdPrefix()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("", isNew);

	ASSERT_TRUE(id.substr(0, 5) == "sess_");
}

// ---------------------------------------------------------------------------
// resolveSession – different instances don't share state
// ---------------------------------------------------------------------------

static void testInstancesAreIndependent()
{
	SessionManager	sm1;
	SessionManager	sm2;
	bool		isNew;

	std::string id1 = sm1.resolveSession("", isNew);

	// sm2 has no knowledge of id1
	std::string cookie = "session_id=" + id1;
	sm2.resolveSession(cookie, isNew);
	ASSERT_TRUE(isNew); // sm2 must treat it as new
}

// ---------------------------------------------------------------------------
// incrementVisits – starts at 1 after first call
// ---------------------------------------------------------------------------

static void testIncrementVisitsStartsAtOne()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("", isNew);
	int visits = sm.incrementVisits(id);

	ASSERT_EQ(1, visits);
}

// ---------------------------------------------------------------------------
// incrementVisits – accumulates correctly across multiple calls
// ---------------------------------------------------------------------------

static void testIncrementVisitsAccumulates()
{
	SessionManager	sm;
	bool		isNew;

	std::string id = sm.resolveSession("", isNew);

	ASSERT_EQ(1, sm.incrementVisits(id));
	ASSERT_EQ(2, sm.incrementVisits(id));
	ASSERT_EQ(3, sm.incrementVisits(id));
	ASSERT_EQ(4, sm.incrementVisits(id));
	ASSERT_EQ(5, sm.incrementVisits(id));
}

// ---------------------------------------------------------------------------
// incrementVisits – two sessions are tracked independently
// ---------------------------------------------------------------------------

static void	testIncrementVisitsTwoSessionsIndependent()
{
	SessionManager	sm;
	bool		isNew;

	std::string idA = sm.resolveSession("", isNew);
	std::string idB = sm.resolveSession("", isNew);

	sm.incrementVisits(idA);
	sm.incrementVisits(idA);
	sm.incrementVisits(idA); // A = 3

	sm.incrementVisits(idB); // B = 1

	ASSERT_EQ(4, sm.incrementVisits(idA)); // A → 4
	ASSERT_EQ(2, sm.incrementVisits(idB)); // B → 2
}

// ---------------------------------------------------------------------------
// Full round-trip: new → cookie → return → increment
// ---------------------------------------------------------------------------

static void	testFullSessionRoundTrip()
{
	SessionManager	sm;
	bool		isNew;

	// First request: no cookie → new session
	std::string id = sm.resolveSession("", isNew);
	ASSERT_TRUE(isNew);
	ASSERT_TRUE(!id.empty());

	// Server sends Set-Cookie: session_id=<id>
	// Second request: browser echoes the cookie back
	std::string cookieHeader = "session_id=" + id;
	std::string sameId = sm.resolveSession(cookieHeader, isNew);
	ASSERT_EQ(id, sameId);
	ASSERT_TRUE(!isNew);

	// Server increments visit counter
	int v1 = sm.incrementVisits(sameId);
	int v2 = sm.incrementVisits(sameId);
	ASSERT_EQ(1, v1);
	ASSERT_EQ(2, v2);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void	testSessionManager()
{
	RUN_TEST(testResolveEmptyCookieCreatesSession);
	RUN_TEST(testResolveUnknownCookieCreatesSession);
	RUN_TEST(testResolveKnownCookieReturnsSameId);
	RUN_TEST(testResolveProducesUniqueIds);
	RUN_TEST(testResolveMultipleCookiesSessionFirst);
	RUN_TEST(testResolveMultipleCookiesSessionLast);
	RUN_TEST(testResolveMultipleCookiesSessionMiddle);
	RUN_TEST(testResolveNoSessionIdKey);
	RUN_TEST(testGeneratedIdPrefix);
	RUN_TEST(testInstancesAreIndependent);
	RUN_TEST(testIncrementVisitsStartsAtOne);
	RUN_TEST(testIncrementVisitsAccumulates);
	RUN_TEST(testIncrementVisitsTwoSessionsIndependent);
	RUN_TEST(testFullSessionRoundTrip);
}
