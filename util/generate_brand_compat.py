from pathlib import Path
import sys

from rdoc_brand import (
    COMPAT_APP_HEADER_NAME,
    COMPAT_BASE_NAME,
    COMPAT_BASE_NAME_UPPER,
    COMPAT_GETAPI_NAME,
    COMPAT_REPLAY_HEADER_NAME,
    COMPAT_REPLAY_MARKER_NAME,
    IN_APPLICATION_API_URL,
    PRODUCT_NAME,
)

ROOT = Path(__file__).resolve().parents[1]

REPLACEMENTS = {
    '@RDOC_COMPAT_BASE_NAME@': COMPAT_BASE_NAME,
    '@RDOC_COMPAT_BASE_NAME_UPPER@': COMPAT_BASE_NAME_UPPER,
    '@RDOC_COMPAT_APP_HEADER_NAME@': COMPAT_APP_HEADER_NAME,
    '@RDOC_COMPAT_REPLAY_HEADER_NAME@': COMPAT_REPLAY_HEADER_NAME,
    '@RDOC_COMPAT_GETAPI_NAME@': COMPAT_GETAPI_NAME,
    '@RDOC_COMPAT_REPLAY_MARKER_NAME@': COMPAT_REPLAY_MARKER_NAME,
}

FILES = [
    ('renderdoc/api/app/renderdoc_app.h', f'renderdoc/api/app/{COMPAT_APP_HEADER_NAME}'),
    ('renderdoc/api/replay/compat_replay.h.in', f'renderdoc/api/replay/{COMPAT_REPLAY_HEADER_NAME}'),
    ('renderdoc/api/replay/compat_config.h.in', 'renderdoc/api/replay/compat_config.h'),
    ('renderdoc/rdocself.version.in', 'renderdoc/rdocself.version'),
]


def render_template(path: Path) -> str:
    text = path.read_text()
    for key, value in REPLACEMENTS.items():
        text = text.replace(key, value)
    return text


def render_app_header(path: Path) -> str:
    text = path.read_text()
    text = text.replace('RENDERDOC_', COMPAT_BASE_NAME_UPPER + '_')
    text = text.replace('RenderDoc', PRODUCT_NAME)
    text = text.replace('renderdoc_app.h', COMPAT_APP_HEADER_NAME)
    text = text.replace('https://renderdoc.org/docs/in_application_api.html', IN_APPLICATION_API_URL)
    return text


def main() -> int:
    check = '--check' in sys.argv[1:]
    dirty = []

    for src_rel, dst_rel in FILES:
        src = ROOT / src_rel
        dst = ROOT / dst_rel
        if src_rel.endswith('renderdoc_app.h'):
            rendered = render_app_header(src)
        else:
            rendered = render_template(src)

        if check:
            if not dst.exists() or dst.read_text() != rendered:
                dirty.append(dst_rel)
            continue

        dst.write_text(rendered)

    if check and dirty:
        for path in dirty:
            print(path)
        return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
