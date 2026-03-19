import json


def save_settings(path, data: dict) -> None:
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def load_settings(path) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)
