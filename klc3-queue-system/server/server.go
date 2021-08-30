package server

import (
	"github.com/gin-gonic/gin"
	"github.com/sirupsen/logrus"
)

func Listen(addr string) error {
	r := gin.Default()
	r.LoadHTMLGlob("templates/*")
	r.GET("/ping", pingHandler)
	r.GET("/status", statusHandler)
	r.GET("/gradetest", gradeTestHandler)
	r.POST("/webhook", webhookHandler)
	r.Any("/queue", oauthHandler)
	logrus.Info("Starting server at ", addr)
	return r.Run(addr)
}