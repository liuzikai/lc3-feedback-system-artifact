package server

import (
	"autograder/klc3"
	"autograder/utils"
	"fmt"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/sirupsen/logrus"
	"gopkg.in/go-playground/webhooks.v5/github"
)

var hook *github.Webhook
var repoWhiteList = map[string]bool{"autograder": true, "_release": true, "klc3Storage": true, "lc3webtool": true, "feedback": true}
var netidWhiteList = map[string]bool{"wenqing4": true, "qili8": true, "tingkai2": true, "zikail2": true, "lumetta": true}

func init() {
	var err error
	logrus.Info("Init github webhook...")
	hook, err = github.New(github.Options.Secret("zjuiece220zjui"))
	if err != nil {
		logrus.Fatal(err)
	}
}

func pingHandler(c *gin.Context) {
	c.JSON(200, gin.H{
		"message": "pong",
	})
}

func statusHandler(c *gin.Context) {
	c.JSON(200, gin.H{
		"message":     "ok",
		"waiting_num": len(klc3.Queue),
		"status":      klc3.GetGradeStatus(),
	})
}

func gradeHandler(payload *github.PushPayload) error {
	netid := payload.Pusher.Name
	// if _, ok := netidWhiteList[netid]; !ok {
	// 	// Only grade whitelist for now
	// 	return nil
	// }
	// We ignore grading whitelist repo
	if _, ok := repoWhiteList[payload.Repository.Name]; ok {
		return nil
	}
	if payload.Ref != "refs/heads/master" {
		// ignore other commit
		return nil
	}
	logrus.Infof("receiving push webhook from %s - %s", netid, payload.HeadCommit.ID)
	klc3.Queue <- &klc3.GradeTask{
		Netid:    netid,
		IsManual: false,
		Payload:  payload,
	}
	return nil
}

func gradeTestHandler(c *gin.Context) {
	netid := c.Query("netid")
	mpStr := c.Query("mp")
	manualList := []string{"MP1", "MP2", "MP3"}
	if mpStr != "" {
		manualList = strings.Split(mpStr,",")
	}

	for idx, s := range manualList {
		manualList[idx] = strings.ToUpper(s)
		if _, ok := utils.MPList[manualList[idx]]; !ok {
			c.JSON(400, gin.H{
				"message": fmt.Sprintf("unsupport mp %s", s),
			})
			return
		}
	}
	logrus.Infof("receiving gradetest from %s", netid)
	klc3.Queue <- &klc3.GradeTask{
		Netid:    netid,
		IsManual: true,
		ManualList: manualList,
		Payload:  nil,
	}
	c.JSON(200, gin.H{
		"message": "OK",
		"data": manualList,
	})
}

func webhookHandler(c *gin.Context) {
	payload, err := hook.Parse(c.Request, github.PushEvent)
	if err != nil {
		_ = c.AbortWithError(400, err)
	}
	switch payload.(type) {
	case github.PushPayload:
		push := payload.(github.PushPayload)
		if err := gradeHandler(&push); err != nil {
			_ = c.AbortWithError(500, err)
		}
		c.JSON(200, gin.H{
			"message": "OK",
		})
	}
}
