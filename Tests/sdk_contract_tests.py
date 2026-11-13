"""Linux-only script contract checks; fake tools do NOT validate an Apple SDK."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='snow-sdk-contract-') as t:
    tmp=Path(t); tools=tmp/'tools';tools.mkdir()
    default=tmp/'default-sdk'; chosen=tmp/'chosen SDK'
    for sdk in (default,chosen): (sdk/'System/Library/Frameworks/AppKit.framework').mkdir(parents=True)
    def executable(name,text):
        path=tools/name;path.write_text('#!/bin/bash\n'+text);path.chmod(0o755);return path
    executable('uname','printf "Darwin\\n"\n')
    compiler=executable('fake-cxx','printf "%s\\n" "$@" > "$LOG"\n/bin/cat > "$INPUT"\nexit "${FAIL_VECTOR:-0}"\n')
    executable('xcrun',f'[[ "$1" == --sdk && "$2" == macosx ]] || exit 9\ncase "$3" in --show-sdk-path) printf "%s\\n" "$DEFAULT_SDK";; --find) printf "%s\\n" "{compiler}";; *) exit 9;; esac\n')
    env=dict(os.environ,PATH=str(tools)+':/usr/bin:/bin',LOG=str(tmp/'args'),INPUT=str(tmp/'input'),DEFAULT_SDK=str(default))
    env.pop('SDKROOT',None);env.pop('CXX',None)
    def run(e):return subprocess.run(['bash','-c','set -eu; source ./sdk.command; printf "SDK=%s\\n" "$SDKROOT"'],cwd=root,env=e,capture_output=True,text=True)
    r=run(env);assert r.returncode==0 and str(default) in r.stdout
    assert '#include <vector>' in (tmp/'input').read_text()
    assert '-isysroot\n'+str(default)+'\n' in (tmp/'args').read_text()
    print('PASS SDK script selects explicit macosx toolchain and preflights vector')
    r=run(dict(env,SDKROOT=str(chosen)));assert r.returncode==0 and str(chosen) in r.stdout
    assert '-isysroot\n'+str(chosen)+'\n' in (tmp/'args').read_text()
    print('PASS SDK script preserves explicit SDKROOT including spaces')
    r=run(dict(env,SDKROOT=str(tmp/'missing')));assert r.returncode!=0 and 'SDKROOT' in r.stderr
    print('PASS invalid SDKROOT rejected without replacing user choice')
    r=run(dict(env,FAIL_VECTOR='1'));assert r.returncode!=0 and '<vector>' in r.stderr and 'xcode-select -p' in r.stderr
    assert 'rm -rf' not in r.stderr
    print('PASS vector failure yields actionable nondestructive toolchain diagnostics')
