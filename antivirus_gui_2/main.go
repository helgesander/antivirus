package main

import (
	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/layout"
	"fyne.io/fyne/v2/widget"
	"github.com/helgesander/antivirus/antivirus_gui/resources"
)

// var topWindow fyne.Window

func main() {
	a := app.NewWithID("helgesander.antivirus")
	a.SetIcon(resources.AntivirusIco)
	w := a.NewWindow("Kaspersky v2.0")
	// makeSystemTray(a)
	// Initialize buttons
	scanFileButton := widget.NewButton("Сканировать", func() {})
	scanFolderButton := widget.NewButton("Сканировать", func() {})
	scanFileLabel := widget.NewLabel("Введите путь для сканируемого файла:")
	scanFolderLabel := widget.NewLabel("Введите путь до сканируемой папки:")
	fileEntry := widget.NewEntry()
	folderEntry := widget.NewEntry()
	scanFileMenuContainer := container.NewVBox(
		layout.NewSpacer(),
		scanFileLabel,
		fileEntry,
		scanFileButton,
		layout.NewSpacer(),
	)
	scanFolderMenuContainer := container.NewVBox(scanFolderLabel, folderEntry, scanFolderButton)
	scanFileMenuButton := widget.NewButton("Сканировать файл", func() {
		scanFileMenuContainer.Show()
		scanFolderMenuContainer.Hide()
		w.Hide()
	})
	scanFolderMenuButton := widget.NewButton("Сканировать папку", func() {
		scanFileMenuContainer.Hide()
		scanFolderMenuContainer.Show()
	})
	buttonBox := container.NewVBox(
		layout.NewSpacer(),
		scanFileMenuButton,
		layout.NewSpacer(),
		scanFolderMenuButton,
		layout.NewSpacer(),
	)
	scanFileMenuContainer.Hide()
	scanFolderMenuContainer.Hide()
	menuContainder := container.NewVBox(scanFileMenuContainer, scanFolderMenuContainer)
	// scanFileMenuContainer.Hide()
	contentBox := container.NewHSplit(buttonBox, menuContainder)
	contentBox.Offset = 0.2

	w.SetMaster()
	w.Resize(fyne.NewSize(640, 460))
	w.SetFixedSize(true)
	w.SetContent(contentBox)
	w.ShowAndRun()
}

// func makeSystemTray(a fyne.App) {
// 	if desk, ok := a.(desktop.App); ok {
// 		h := fyne.NewMenuItem("Hello", func() {})
// 		h.Icon = theme.HomeIcon()
// 		menu := fyne.NewMenu("Hello World", h)
// 		h.Action = func() {
// 			h.Label = "Welcome"
// 			menu.Refresh()
// 		}
// 		desk.SetSystemTrayMenu(menu)
// 	}
// }
