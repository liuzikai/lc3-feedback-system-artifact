package klc3

import (
	"errors"
	"fmt"
	"github.com/sirupsen/logrus"
	"os"
	"os/exec"
	"strings"
)

var ErrKlc3Internal = errors.New("klc3 internal error")
var ErrStudentCode = errors.New("student code error")

func ExecTest() {
	cmd := exec.Command("/usr/local/bin/python3", "hello.py")
	cmd.Dir = "/Users/laphets/code/ece220-staff"
	cmd.Env = os.Environ()
	cmd.Env = append(cmd.Env, "LD_LIBRARY_PATH=/opt/rh/llvm-toolset-7.0/root/usr/lib64")

	out, err := cmd.Output()
	if err != nil {
		if exitError, ok := err.(*exec.ExitError); ok {
			logrus.Error(exitError.ExitCode())
		}
	}
	logrus.Info(string(out))
}

func ExecMP(MP string, mpPath string, outputPath string, concreteDir string) error {

	cmd := exec.Command("/usr/bin/python3", "test.py", fmt.Sprintf("--output-dir=%s", outputPath), fmt.Sprintf("--regression-dir=%s", concreteDir), mpPath)
	cmd.Dir = fmt.Sprintf("/home/klc3/klc3/examples/%s", strings.ToLower(MP))
	cmd.Env = os.Environ()
	cmd.Env = append(cmd.Env, "LD_LIBRARY_PATH=/opt/rh/llvm-toolset-7.0/root/usr/lib64")
	logrus.Info(cmd.String())

	output, err := cmd.Output()
	
	if err != nil {
		logrus.Error(err, string(output))
		return err
		// if exitError, ok := err.(*exec.ExitError); ok {
		// 	if exitError.ExitCode() == 255 {
		// 		return ErrKlc3Internal
		// 	} else {
		// 		return ErrStudentCode
		// 	}
		// }
	}
	return nil
}

func ExecMP1Concrete(mp1Path string, outputPath string, concretePath string) error {
	cmd := exec.Command("/usr/bin/python3", "test.py", fmt.Sprintf("--output-dir=%s", outputPath), fmt.Sprintf("--input-data-dir=%s", concretePath), mp1Path)
	logrus.Info(cmd.String())
	cmd.Dir = "/home/klc3/klc3/examples/mp1"
	cmd.Env = os.Environ()
	cmd.Env = append(cmd.Env, "LD_LIBRARY_PATH=/opt/rh/llvm-toolset-7.0/root/usr/lib64")

	output, err := cmd.Output()
	if err != nil {
		logrus.Error(err, output)
		if exitError, ok := err.(*exec.ExitError); ok {
			if exitError.ExitCode() == 255 {
				return ErrKlc3Internal
			} else {
				return ErrStudentCode
			}
		}
	}
	return nil
}
