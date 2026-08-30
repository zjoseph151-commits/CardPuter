Import("env")


ESPTOOL_COMPAT_REPLACEMENTS = (
    ("write-flash", "write_flash"),
    ("erase-flash", "erase_flash"),
    ("--flash-mode", "--flash_mode"),
    ("--flash-freq", "--flash_freq"),
    ("--flash-size", "--flash_size"),
    ("no-reset-no-sync", "no_reset_no_sync"),
    ("no-reset-stub", "no_reset_stub"),
    ("default-reset", "default_reset"),
    ("usb-reset", "usb_reset"),
    ("hard-reset", "hard_reset"),
    ("soft-reset", "soft_reset"),
    ("no-reset", "no_reset"),
)


def _patch_text(value):
    for old, new in ESPTOOL_COMPAT_REPLACEMENTS:
        value = value.replace(old, new)
    return value


def _patch_list(values):
    return [_patch_text(value) if isinstance(value, str) else value for value in values]


def _patch_builder_action(builder):
    action = getattr(builder, "action", None)
    cmd_list = getattr(action, "cmd_list", None)
    if isinstance(cmd_list, str):
        action.cmd_list = _patch_text(cmd_list)


if "_ESPTOOL_COMPAT_ORIGINAL_VERBOSE_ACTION" not in env:
    env["_ESPTOOL_COMPAT_ORIGINAL_VERBOSE_ACTION"] = env.VerboseAction

    def _compat_verbose_action(self, action, *args, **kwargs):
        if isinstance(action, str):
            action = _patch_text(action)
        elif isinstance(action, list):
            action = _patch_list(action)
        return self["_ESPTOOL_COMPAT_ORIGINAL_VERBOSE_ACTION"](action, *args, **kwargs)

    env.AddMethod(_compat_verbose_action, "VerboseAction")

builders = env.get("BUILDERS", {})
if "ElfToBin" in builders:
    _patch_builder_action(builders["ElfToBin"])

for key in ("ERASECMD", "UPLOADCMD"):
    if key in env and isinstance(env[key], str):
        env[key] = _patch_text(env[key])

for key in ("ERASEFLAGS", "UPLOADERFLAGS"):
    if key in env and isinstance(env[key], list):
        env[key] = _patch_list(env[key])
