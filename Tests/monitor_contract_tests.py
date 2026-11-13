#!/usr/bin/env python3
"""Portable source contracts for native-only menu/persistence wiring.
These checks do NOT replace AppKit/Windows GUI runtime acceptance.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
if (root/'Windows/main.cpp').exists():
    s=(root/'Windows/main.cpp').read_text()
    assert 'L"Мониторы"' in s and 'MF_CHECKED' in s
    assert 'MONITORINFOEX' in s and 'mi.szDevice' in s
    assert 'GetPrivateProfileInt(L"Monitors",key.c_str(),1' in s
    assert 'WritePrivateProfileString(L"Monitors",choice.key.c_str()' in s
    rebuild=s.split('void displays(){',1)[1].split('BOOL CALLBACK enumerate',1)[0]
    assert 'DestroyWindow(control)' not in rebuild and 'snow_forget_windows' not in rebuild
    assert 'if(overlays.empty())return 0;' in s
    assert 'overlays.empty()?500:16' in s and 'case WM_DISPLAYCHANGE:if(h==control)displays();' in s
    assert 'ReleaseCapture();return 0;}auto now=' in s
    assert 'DeletePrivateProfile' not in s # retain absent monitor choices
    print('PASS Windows checkable monitor submenu / deviceName INI default-on / missing choices retained / tray survives last disable / idle / capture cancelled')
else:
    s=(root/'Sources/main.swift').read_text()
    assert 'title:"Мониторы"' in s and 'entry.state=enabled ? .on:.off' in s
    assert 'CGDisplayCreateUUIDFromDisplayID' in s
    assert 'UserDefaults.standard.stringArray(forKey:"DisabledDisplayUUIDs") ?? []' in s
    assert 'UserDefaults.standard.set(Array(newValue).sorted(),forKey:"DisabledDisplayUUIDs")' in s
    assert 'if disabled.contains(key){disabled.remove(key)}else{disabled.insert(key)}' in s
    rebuild=s.split('@objc func rebuildDisplays(){',1)[1].split('func snapshot(){',1)[0]
    assert 'status' not in rebuild and 'snow_forget_windows' not in rebuild
    assert 'if screenRects.isEmpty {return}' in s
    assert 'screenRects.isEmpty ? 0.5 : 1.0/60' in s
    assert 'if !enabled {disabledRects.append' in s
    print('PASS macOS checkable monitor submenu / persistent display UUID defaults-on / missing choices retained / status item survives last disable / idle')
assert 'snow_disabled_displays' in s and 'snow_displays' in s
print('PASS native wiring supplies disabled physical rectangles plus enabled physics floors; no global reset on monitor toggle')
