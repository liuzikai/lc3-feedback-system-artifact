package main

import (
	"autograder/klc3"
	"autograder/repo"
	"autograder/server"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"time"

	"github.com/sirupsen/logrus"
	"gopkg.in/go-playground/webhooks.v5/github"
)

func pushHandler(payload *github.PushPayload) error {
	fmt.Printf("%+v", payload)
	return nil
}

// func grade(netid string) error {
// 	studentRepo, err := repo.GetStudentRepo("wenqing4")
// 	if err != nil {
// 		return err
// 	}

// 	if err := studentRepo.GenerateGrade(); err != nil {
// 		return err
// 	}

// 	return nil
// }

func init() {
	// init logrus
	logrus.SetReportCaller(true)
	logrus.SetFormatter(&logrus.JSONFormatter{})
	file, err := os.OpenFile("server.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0666)
	if err != nil {
		logrus.Fatal(err)
	}
	mw := io.MultiWriter(os.Stdout, file)
	logrus.SetOutput(mw)
}

func run() error {
	tmpDir, err := filepath.Abs(("./tmp"))
	if err != nil {
		return err
	}
	_ = os.Mkdir(tmpDir, 0755)
	dir, err := ioutil.TempDir(tmpDir, "output")
	if err != nil {
		return err
	}

	err = klc3.ExecMP("MP1", "/home/INTL/wenqing.17/mp1.asm", dir, dir)
	if err != nil {
		return err
	}

	reportByte, err := ioutil.ReadFile(filepath.Join(dir, "report.md"))
	if err != nil {
		return err
	}
	report := string(reportByte)

	if err := ioutil.WriteFile(filepath.Join(dir, "README.md"), []byte(report), 0644); err != nil {
		return err
	}
	//defer os.RemoveAll(dir)

	return nil

}

func main() {
	// if err := run(); err != nil {
	// 	logrus.Error(err)
	// }

	klc3Repo, err := repo.GetKlc3Repo()
	if err != nil {
		logrus.Fatal(err)
	}
	if err := klc3Repo.AutoSync(); err != nil {
		logrus.Fatal(err)
	}

	if err := klc3.StartQueue(4, 400, time.Minute*10); err != nil {
		logrus.Fatal(err)
	}
	if err := server.Listen("0.0.0.0:8080"); err != nil {
		logrus.Fatal(err)
	}

}
