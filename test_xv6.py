import pexpect
import sys

def main():
    print("Spawning qemu...")
    child = pexpect.spawn('make qemu', timeout=5, encoding='utf-8')
    child.logfile = sys.stdout
    
    try:
        child.expect('init: starting sh')
        print(">>> Booted successfully! <<<")
        
        # Test 1: Set persistent threshold
        child.sendline('powerctl threshold 60')
        child.expect(r'\$ ')
        
        # Shutdown QEMU to test persistence
        print(">>> Shutting down QEMU... <<<")
        child.send('\x01x')
        child.expect(pexpect.EOF)
        
        # Reboot
        print(">>> Rebooting QEMU... <<<")
        child = pexpect.spawn('make qemu', timeout=5, encoding='utf-8')
        child.logfile = sys.stdout
        
        child.expect('init: starting sh')
        child.sendline('powerctl status')
        child.expect('threshold=60')
        print(">>> Verified persistent threshold! <<<")
        
        # Test 2: Background waitpower test
        print(">>> Starting waitpower task <<<")
        child.sendline('waitpower &')
        child.expect(r'\$ ')
        
        # Drain the battery
        child.sendline('powerctl level 10')
        child.expect(r'\$ ')
        child.sendline('powerctl charge 0')
        child.expect(r'\$ ')
        
        # Wait for the power SAVER notification
        child.expect('waitpower: NOTIFIED!', timeout=15)
        print(">>> Notification achieved! <<<")
        
        child.sendline('\x01x')
        child.expect(pexpect.EOF)
        
    except pexpect.TIMEOUT:
        print(">>> Timeout reached! Output up to timeout: <<<")
        print(child.before)
        child.send('\x01x')
    except Exception as e:
        print(">>> Error: <<<", str(e))
        child.send('\x01x')

if __name__ == "__main__":
    main()
