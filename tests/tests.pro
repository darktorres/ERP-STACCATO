#-------------------------------------------------
# SUBDIRS aggregator for test tiers.
# tier1: pure unit tests (no DB, no UI event loop)
# tier2: integration tests against a local MySQL staccato_test (M3)
# tier3: UI smoke tests via QTest::keyClicks/mouseClick (M4)
#
# See .claude/test-infrastructure-plan.md for the full milestone plan.
#-------------------------------------------------

TEMPLATE = subdirs

SUBDIRS = tier1 tier2 tier3
