import unittest

from server import RssiEstimatorService, extract_anchor_id
from sim.config import AnchorConfig


ADVERTISEMENT = "1409626c652d6c6f6361746f722d6573703332633309ff3412123456789abc"


class ServerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.anchors = [
            AnchorConfig("anchor_1", 0.0, 0.0),
            AnchorConfig("anchor_2", 10.0, 0.0),
            AnchorConfig("anchor_3", 10.0, 10.0),
            AnchorConfig("anchor_4", 0.0, 10.0),
        ]
        self.ble_ids = {
            f"anchor_{index}": f"00:00:00:00:00:0{index}"
            for index in range(1, 5)
        }

    def test_extracts_manufacturer_anchor_id(self) -> None:
        self.assertEqual(extract_anchor_id(ADVERTISEMENT), "12:34:56:78:9a:bc")

    def test_estimates_after_all_anchors_are_received(self) -> None:
        service = RssiEstimatorService(self.anchors, self.ble_ids)
        for index in range(1, 4):
            result = service.ingest({"anchor_id": f"anchor_{index}", "rssi": -55})
            self.assertNotIn("estimate", result)

        result = service.ingest({"anchor_id": "anchor_4", "rssi": -55})
        self.assertIn("estimate", result)
        self.assertEqual(set(result["observations"]), set(self.ble_ids))

    def test_accepts_existing_scanner_payload(self) -> None:
        service = RssiEstimatorService(
            self.anchors,
            {"anchor_1": "12:34:56:78:9a:bc"},
        )
        result = service.ingest({"address": "aa:bb:cc:dd:ee:ff", "rssi": -55, "data": ADVERTISEMENT})
        self.assertEqual(result["observations"], {"anchor_1": -55.0})


if __name__ == "__main__":
    unittest.main()