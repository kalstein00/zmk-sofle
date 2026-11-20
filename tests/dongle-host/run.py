"""Run host checks against actual adapters generated from the pinned ZMK tree."""
import argparse
import importlib.util
import pathlib
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True

ROOT = pathlib.Path(__file__).resolve().parents[2]
HERE = pathlib.Path(__file__).resolve().parent


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('zmk_app', type=pathlib.Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('wireless', ROOT/'zephyr/prepare_dongle_wireless.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix='sofle-host-') as directory:
        output = pathlib.Path(directory)
        for name in module.TRANSFORMS:
            relative = {'studio_rpc': 'studio/rpc.c', 'split_central': 'split/central.c',
                        'sticky_key': 'behaviors/behavior_sticky_key.c',
                        'two_axis': 'behaviors/behavior_input_two_axis.c'}.get(name, name+'.c')
            source = args.zmk_app/'src'/relative
            # Invoke the same hash check as the firmware build.
            run('python3', ROOT/'zephyr/prepare_dongle_wireless.py', name, source, output/(name+'.c'))
        hog = (output/'hog.c').read_text()
        start = hog.index('K_THREAD_STACK_DEFINE(hog_q_stack,')
        end = hog.index('static int zmk_hog_init(', start)
        release = hog[hog.index('K_SEM_DEFINE(dongle_release_sent,'):]
        (output/'hog.generated.c').write_text(hog[start:end] + release)
        controller = (ROOT/'zephyr/dongle_host.c').read_text()
        (output/'controller.generated.c').write_text('\n'.join(
            line for line in controller.splitlines() if not line.startswith('#include')))
        endpoints = (output/'endpoints.c').read_text()
        (output/'endpoints.generated.c').write_text('\n'.join(
            line for line in endpoints.splitlines() if not line.startswith('#include')))
        ble = (output/'ble.c').read_text()
        start = ble.index('static void auth_pairing_complete(')
        end = ble.index('static struct bt_conn_auth_cb ', start)
        (output/'pairing.generated.c').write_text(ble[start:end])
        flags = ['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                 '-Wno-unused-variable', '-I'+str(HERE), '-I'+str(output), '-I'+str(ROOT/'zephyr')]
        run(*flags, HERE/'state_test.c', ROOT/'zephyr/dongle_host_state.c', '-o', output/'state_test')
        run(output/'state_test')
        run(*flags, HERE/'hog_test.c', '-o', output/'hog_test')
        run(output/'hog_test')
        run(*flags, '-std=gnu11', HERE/'controller_test.c', ROOT/'zephyr/dongle_host_state.c',
            ROOT/'zephyr/dongle_menu.c',
            '-o', output/'controller_test')
        run(output/'controller_test')
        run(*flags, '-Wno-unused-function', HERE/'endpoints_test.c', '-o', output/'endpoints_test')
        run(output/'endpoints_test')
        run(*flags, '-Wno-unused-function', HERE/'pairing_test.c', '-o', output/'pairing_test')
        run(output/'pairing_test')
        central = output/'split_radio.c'
        run('python3', ROOT/'zephyr/prepare_dongle_central.py',
            args.zmk_app/'src/split/bluetooth/central.c', central)
        source = central.read_text()
        start = source.index('static int start_scanning(void) {')
        retry = source.find('static void dongle_scan_retry(', 0, start)
        end = source.index('static void split_central_connected(', start)
        (output/'scan.generated.c').write_text(source[retry if retry >= 0 else start:end])
        run(*flags, '-Wno-unused-function', HERE/'scan_test.c', '-o', output/'scan_test')
        run(output/'scan_test')
        log = (ROOT/'zephyr/dongle_log.c').read_text()
        (output/'log.generated.c').write_text('\n'.join(
            line for line in log.splitlines() if not line.startswith('#include <zephyr/')))
        run(*flags, '-Wno-unused-function', HERE/'log_test.c', '-o', output/'log_test')
        run(output/'log_test')


if __name__ == '__main__':
    main()
