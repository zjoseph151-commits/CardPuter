from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")
GITIGNORE = (ROOT / ".gitignore").read_text(encoding="utf-8")
PLAN_PATH = ROOT / "docs" / "superpowers" / "plans" / "2026-07-25-raspberry-pi-command-center.md"
PLAN = PLAN_PATH.read_text(encoding="utf-8")
SOURCE = firmware_source_text()
PI_MONITOR_SOURCE = (ROOT / "src" / "pi_monitor.cpp").read_text(encoding="utf-8")
INPUT_SOURCE = (ROOT / "src" / "input.cpp").read_text(encoding="utf-8")
PLATFORMIO = (ROOT / "platformio.ini").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_priority_6_plan_is_documented():
    required_tokens = [
        "Wi-Fi MQTT",
        "Pi Monitor",
        "whitelisted",
        "/config/pi.txt",
        "mqtt_host=10.0.0.180",
        "mqtt_port=1883",
        "device_id=scoober-cardputer",
        "command_target=esp32-c3-test",
        "home/#",
        "home/devices/<device>/<kind>",
        "home/devices/<command_target>/commands",
        "read_now",
        "set_interval",
        "message count",
        "last topic/payload",
        "command response display",
        "home/devices/<device>/responses",
        "home/devices/scoober-cardputer/status",
        "home/devices/scoober-cardputer/availability",
        "target selection",
        "project list",
        "command list",
        "Home / Diagnostics",
        "project=id|Label|profile",
        "Do not implement direct shell control",
    ]

    require_tokens(PLAN, required_tokens, "Priority #6 plan")
    require_tokens(README, required_tokens[:14], "README Priority #6 plan")
    require_tokens(NOTES, required_tokens[:14], "notes Priority #6 plan")
    require_tokens(TODO, required_tokens[:14], "TODO Priority #6 plan")
    response_tokens = [
        "command response display",
        "home/devices/<device>/responses",
        "command_target",
        "Resp:",
    ]
    require_tokens(README, response_tokens, "README Pi response display")
    require_tokens(NOTES, response_tokens, "notes Pi response display")
    require_tokens(TODO, response_tokens, "TODO Pi response display")
    status_tokens = [
        "Cardputer status/availability",
        "home/devices/scoober-cardputer/status",
        "home/devices/scoober-cardputer/availability",
    ]
    require_tokens(PLAN, status_tokens, "Priority #6 status publishing")
    require_tokens(README, status_tokens, "README Pi status publishing")
    require_tokens(NOTES, status_tokens, "notes Pi status publishing")
    require_tokens(TODO, status_tokens, "TODO Pi status publishing")
    command_tokens = [
        "fixed",
        "set_interval",
        "10, 30, 60, and 300 seconds",
        "home/devices/<command_target>/commands",
    ]
    require_tokens(PLAN, command_tokens, "Priority #6 set_interval command")
    require_tokens(README, command_tokens, "README Pi set_interval command")
    require_tokens(NOTES, command_tokens, "notes Pi set_interval command")
    require_tokens(TODO, command_tokens, "TODO Pi set_interval command")
    target_tokens = [
        "target selection",
        "discovered device",
        "command_target",
    ]
    require_tokens(PLAN, target_tokens, "Priority #6 target selection")
    require_tokens(README, target_tokens, "README Pi target selection")
    require_tokens(NOTES, target_tokens, "notes Pi target selection")
    require_tokens(TODO, target_tokens, "TODO Pi target selection")
    project_ui_tokens = [
        "project list",
        "command list",
        "Home / Diagnostics",
        "project=id|Label|profile",
        "manually paged",
        "`S` advances the OLED MQTT message page",
    ]
    require_tokens(PLAN, project_ui_tokens, "Priority #6 project UI")
    require_tokens(README, project_ui_tokens, "README Pi project UI")
    require_tokens(NOTES, project_ui_tokens, "notes Pi project UI")
    require_tokens(TODO, project_ui_tokens, "TODO Pi project UI")


def test_local_pi_config_paths_are_ignored():
    require_tokens(
        GITIGNORE,
        [
            "/config/pi.txt",
            "/pi.txt",
        ],
        ".gitignore Raspberry Pi config ignore",
    )


def test_pi_monitor_firmware_is_implemented():
    required_source_tokens = [
        'PI_CONFIG_PATH = "/config/pi.txt"',
        'PI_MONITOR_DEFAULT_DEVICE_ID = "scoober-cardputer"',
        "Screen::PiMonitor",
        '"Pi Monitor"',
        "showPiMonitor()",
        "renderPiMonitor()",
        "initPiMonitorConfigSd()",
        "readPiMonitorConfigFromSd()",
        "connectPiMonitorMqtt()",
        "disconnectPiMonitorMqtt()",
        "publishPiMonitorReadNowCommand()",
        "publishPiMonitorSetIntervalCommand()",
        "cyclePiMonitorCommandTarget()",
        "cyclePiMonitorSetInterval()",
        "stopPiMonitor()",
        "servicePiMonitor()",
        "clearPiMonitorDevices()",
        "handlePiMonitorMessage(",
        "PiMonitorView",
        "PiMonitorView::ProjectList",
        "PiMonitorView::ProjectCommands",
        "PiMonitorView::Diagnostics",
        "PiMonitorCommandDef",
        "PiMonitorProjectConfig",
        "PiMonitorMessage",
        "MAX_PI_MONITOR_PROJECTS",
        "MAX_PI_MONITOR_MESSAGES",
        "PI_MONITOR_DEFAULT_COMMAND_PROFILE",
        "buildPiMonitorProjectItems",
        "parsePiMonitorProjectConfig",
        "movePiMonitorSelection",
        "enterPiMonitorSelection",
        "returnPiMonitorProjectList",
        "cyclePiMonitorCommandOption",
        "publishSelectedPiMonitorCommand",
        "recordPiMonitorMessage",
        "PI_MONITOR_MESSAGE_TOPIC_MAX_CHARS",
        "PI_MONITOR_MESSAGE_PAYLOAD_MAX_CHARS",
        "PI_MONITOR_OLED_MESSAGE_LINE_COUNT",
        "PI_MONITOR_OLED_MESSAGE_BODY_LINES",
        "PI_MONITOR_OLED_MESSAGE_MAX_CHARS",
        "piMonitorLatestOledMessageText",
        "piMonitorLatestOledMessageSequence",
        "advancePiMonitorOledMessagePage",
        "piMonitorFilteredMessageCount",
        "Home / Diagnostics",
        'key == "mqtt_host"',
        'key == "mqtt_port"',
        'key == "device_id"',
        'key == "command_target"',
        'key == "project"',
        'subscribe("home/#")',
        "PI_MONITOR_READ_NOW_PAYLOAD",
        "PI_MONITOR_SET_INTERVAL_OPTIONS_SECONDS",
        "PI_MONITOR_STATUS_PUBLISH_INTERVAL_MS",
        "piMonitorSelectedCommandTarget",
        "activePiMonitorCommandTarget()",
        "buildPiMonitorTargetCandidates",
        "isPiMonitorOwnDevice",
        'key == "command_target"',
        "key == 't' || key == 'T'",
        "key == ';' || key == ','",
        "key == '.' || key == '/'",
        "key == 's' || key == 'S'",
        'String("home/devices/") + commandTarget + "/commands"',
        "piMonitorMqttClient.publish(commandTopic.c_str(), PI_MONITOR_READ_NOW_PAYLOAD)",
        "buildPiMonitorSetIntervalPayload",
        'document["command"] = "set_interval"',
        'document["seconds"] = selectedPiMonitorSetIntervalSeconds()',
        "piMonitorMqttClient.publish(commandTopic.c_str(), commandPayload)",
        "read_now sent",
        "set_interval command published",
        "set_interval failed.",
        "No command target.",
        "buildPiMonitorDeviceTopic",
        'buildPiMonitorDeviceTopic("availability")',
        'buildPiMonitorDeviceTopic("status")',
        "publishPiMonitorAvailability",
        "publishPiMonitorStatus",
        'connect(clientId.c_str(), availabilityTopic.c_str(), 0, true,',
        'publishPiMonitorAvailability("online")',
        'publishPiMonitorAvailability("offline")',
        'piMonitorMqttClient.publish(availabilityTopic.c_str(), availability, true)',
        'piMonitorMqttClient.publish(statusTopic.c_str(), statusPayload, true)',
        'document["firmware_version"]',
        'document["wifi_rssi"]',
        'document["free_heap"]',
        "isPiMonitorResponseKind(topicKind)",
        "isPiMonitorCommandTargetUpdate(deviceId, topicKind)",
        "PI_MONITOR_RESPONSE_WINDOW_MS",
        "piMonitorLastCommandSentMs",
        "summarizeResponsePayload(payloadText)",
        "piMonitorLastResponseSummary",
        "piMonitorResponseCount",
        "Resp: %s",
        "piMonitorLastTopic",
        "piMonitorLastPayload",
        "piMonitorMessageCount",
        "Use WiFi Connect.",
        "No pi config.",
        "Missing mqtt_host.",
        "Waiting for MQTT data.",
    ]

    require_tokens(SOURCE, required_source_tokens, "Pi Monitor firmware")
    require_tokens(
        PLATFORMIO,
        [
            "knolleary/PubSubClient",
            "bblanchon/ArduinoJson",
        ],
        "Pi Monitor dependencies",
    )


def test_pi_monitor_publishing_is_scoped():
    assert PI_MONITOR_SOURCE.count(".publish(") == 4, (
        "Pi Monitor should only publish availability, status, read_now, and set_interval"
    )
    assert PI_MONITOR_SOURCE.count(
        "piMonitorMqttClient.publish(commandTopic.c_str(), PI_MONITOR_READ_NOW_PAYLOAD)"
    ) == 1, "Pi Monitor should have one command publish path"
    assert PI_MONITOR_SOURCE.count(
        "piMonitorMqttClient.publish(commandTopic.c_str(), commandPayload)"
    ) == 1, "Pi Monitor should have one set_interval publish path"

    forbidden_source_tokens = [
        '"home/devices/+/commands"',
        '"home/devices/+/responses"',
        "PI_MONITOR_OLED_PAGE_INTERVAL_MS",
        "now - lastPiMonitorOledPageMs",
        "ssh",
        "shell",
        "sudo",
        "reboot",
    ]

    for token in forbidden_source_tokens:
        assert token not in PI_MONITOR_SOURCE, (
            f"Pi Monitor command publishing must stay tightly scoped: {token}"
        )

    assert "advancePiMonitorOledMessagePage();" in INPUT_SOURCE, (
        "Pi Monitor S key should manually advance the OLED MQTT page"
    )
    assert "publishPiMonitorSetIntervalCommand();" not in INPUT_SOURCE, (
        "Pi Monitor set_interval should be sent from the selected command path, not S"
    )


if __name__ == "__main__":
    test_priority_6_plan_is_documented()
    test_local_pi_config_paths_are_ignored()
    test_pi_monitor_firmware_is_implemented()
    test_pi_monitor_publishing_is_scoped()
    print("Raspberry Pi command center plan checks passed.")
