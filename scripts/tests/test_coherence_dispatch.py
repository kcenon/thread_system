import copy
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("dispatch", Path(__file__).parents[1] / "coherence_dispatch.py")
dispatch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dispatch)


class DispatchTests(unittest.TestCase):
    def setUp(self):
        self.value = dispatch.payload("thread_system", "a"*40, "b"*40, dispatch.REQUEST, "a"*40)
        self.run = {"id": 17, "html_url": "https://github.com/kcenon/thread_system/actions/runs/17",
                    "status": "completed", "conclusion": "success"}
        self.result = dict(self.value, repository="thread_system", source_sha="a"*40, raw_exit_code=0)

    def test_allowlist_pins_origin_and_correlation(self):
        for key, value in [("candidate_repository", "elsewhere"), ("candidate_sha", "develop"),
                           ("lock_revision", "abc"), ("origin", "kcenon/pacs_system"), ("correlation", "wrong")]:
            invalid = dict(self.value, **{key:value})
            with self.assertRaises(ValueError): dispatch.validate_payload(dispatch.REQUEST, invalid)
        with self.assertRaises(ValueError): dispatch.validate_payload(dispatch.CHANGE, self.value)
        with self.assertRaises(ValueError): dispatch.validate_payload("coherence-result-v1", self.value)

    def test_receiver_uses_changed_repository_and_locked_siblings(self):
        event = {"action": dispatch.REQUEST, "client_payload": self.value}
        with patch.object(dispatch, "locked_sources", return_value={repo:"c"*40 for repo in dispatch.REPOS}):
            self.assertEqual(dispatch.validate_receiver(event,"thread_system")["receiver_sha"],"a"*40)
            with self.assertRaises(ValueError): dispatch.validate_receiver(event,"pacs_system")
            event["client_payload"] = dict(self.value, receiver_sha="c"*40)
            dispatch.validate_receiver(event,"pacs_system")

    def test_raw_result_must_match_tuple_and_checked_commit(self):
        dispatch.check_result(self.result, self.value, "thread_system")
        for key, value in [("raw_exit_code",1),("raw_exit_code",False),("raw_exit_code",None),
                           ("source_sha","d"*40),("lock_revision","d"*40),("correlation","bad")]:
            with self.assertRaises(ValueError):
                dispatch.check_result(dict(self.result, **{key:value}), self.value, "thread_system")

    def test_duplicate_request_reuses_run_and_validates_artifact(self):
        with patch.object(dispatch,"matching_runs",return_value=[self.run]), \
             patch.object(dispatch,"artifact_result",return_value=self.result), patch.object(dispatch,"api") as api:
            runs = dispatch.dispatch_and_wait("thread_system",dispatch.REQUEST,self.value,["coherence-receiver.yml"])
            self.assertEqual(runs["coherence-receiver.yml"],self.run["html_url"])
            api.assert_not_called()

    def test_accepted_delivery_times_out_without_receiver(self):
        with patch.object(dispatch,"matching_runs",return_value=[]), patch.object(dispatch,"api") as api:
            with self.assertRaisesRegex(ValueError,"timed out"):
                dispatch.dispatch_and_wait("thread_system",dispatch.REQUEST,self.value,["coherence-receiver.yml"],
                                           timeout=1, now=iter([0,0,2,2]).__next__, sleep=lambda _:None)
            self.assertEqual(api.call_args.args[0],"repos/kcenon/thread_system/dispatches")

    def test_authentication_and_receiver_failure_are_not_success(self):
        with patch.object(dispatch,"matching_runs",return_value=[]), patch.object(dispatch,"api",side_effect=ValueError("HTTP 403")):
            with self.assertRaisesRegex(ValueError,"403"):
                dispatch.dispatch_and_wait("thread_system",dispatch.REQUEST,self.value,["coherence-receiver.yml"])
        for conclusion in ("failure","cancelled","skipped","timed_out"):
            with patch.object(dispatch,"matching_runs",return_value=[dict(self.run,conclusion=conclusion)]):
                with self.assertRaisesRegex(ValueError,conclusion):
                    dispatch.dispatch_and_wait("thread_system",dispatch.REQUEST,self.value,["coherence-receiver.yml"])


if __name__ == "__main__":
    unittest.main()
