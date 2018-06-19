/*
    RPG Paper Maker Copyright (C) 2017 Marie Laporte

    This file is part of RPG Paper Maker.

    RPG Paper Maker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPG Paper Maker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "panelsongs.h"
#include "ui_panelsongs.h"
#include "wanok.h"
#include "common.h"
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

PanelSongs::PanelSongs(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PanelSongs),
    m_songKind(SongKind::None),
    m_song(nullptr)
{
    ui->setupUi(this);

    ui->widgetPanelIDs->showButtonMax(false);
    ui->widgetPanelIDs->list()->setCanBrutRemove(true);
    ui->widgetPanelIDs->list()->setHasContextMenu(false);
    connect(ui->checkBoxContent, SIGNAL(toggled(bool)),
            this, SLOT(showAvailableContent(bool)));

    // Available content
    ui->treeViewAvailableContent->initializeModel(new QStandardItemModel);
    ui->treeViewAvailableContent->setHasContextMenu(false);
    ui->treeViewAvailableContent->setCanBeControled(false);
    ui->treeViewAvailableContent->setCanMove(false);
    ui->treeViewAvailableContent->setUpdateId(true);

    connect(ui->treeViewAvailableContent,
            SIGNAL(deletingItem(SuperListItem*, int)),
            this,
            SLOT(deletingContent(SuperListItem*, int)));
    connect(ui->widgetPanelIDs->list(), SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(on_widgetPanelIDsDoubleClicked(QModelIndex)));
    connect(&m_mediaPlayer,
            SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this,
            SLOT(on_mediaStatusChanged(QMediaPlayer::MediaStatus)));
}

PanelSongs::~PanelSongs()
{
    SuperListItem::deleteModel(ui->treeViewAvailableContent->getModel());
    m_mediaPlayer.stop();
    delete ui;
}

SystemSong* PanelSongs::song() const { return m_song; }

void PanelSongs::setSong(SystemSong* song) {
    m_song = song;
}

// -------------------------------------------------------
//
//  INTERMEDIARY FUNCTIONS
//
// -------------------------------------------------------

void PanelSongs::setSongKind(SongKind kind){
    bool isNone = kind == SongKind::None;
    m_songKind = kind;

    showSongs(!isNone);

    if (!isNone){
        ui->widgetPanelIDs->initializeModel(
                    Wanok::get()->project()->songsDatas()->model(kind));

        // Connection of list
        connect(ui->widgetPanelIDs->list()->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(on_listIDsIndexChanged(QModelIndex,QModelIndex)));

        QModelIndex index = ui->widgetPanelIDs->list()->getModel()->index(0, 0);
        ui->widgetPanelIDs->list()->setCurrentIndex(index);


        // Loading first available content
        loadAvailableContent(-2);

        connect(ui->treeViewAvailableContent->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(on_listIndexChanged(QModelIndex,QModelIndex)));

        // Update checkBox
        ui->checkBoxContent->setText("Show available content of " +
                                     SystemSong::getLocalFolder(kind));
    }
}

// -------------------------------------------------------

void PanelSongs::changeSong(SystemSong* song){
    setSong(song);

    int index = SuperListItem::getIndexById(ui->widgetPanelIDs->list()
                                            ->getModel()->invisibleRootItem(),
                                            song->id());
    QModelIndex indexModel = ui->widgetPanelIDs->list()->getModel()
            ->index(index, 0);
    ui->widgetPanelIDs->list()->setCurrentIndex(indexModel);
}

// -------------------------------------------------------

void PanelSongs::showSongs(bool b) {
    this->setVisible(b);
}

// -------------------------------------------------------

void PanelSongs::updateSong(QStandardItem* item) {
    if (item != nullptr) {
        m_song = (SystemSong*) item->data().value<qintptr>();
        if (m_song != nullptr) {
            if (m_song->id() != -1) {
                m_mediaPlayer.setMedia(
                    QUrl::fromLocalFile(m_song->getPath(m_songKind)));
            }
            ui->pushButtonPlay->setEnabled(m_song->id() != -1);
        }
    }
}

// -------------------------------------------------------

void PanelSongs::loadAvailableContent(int row){
    if (row == -1){
        QStandardItem* item = ui->treeViewAvailableContent->getSelected();
        if (item != nullptr)
            row = item->row();
    }

    // Clear
    SuperListItem::deleteModel(ui->treeViewAvailableContent->getModel(), false);
    ui->treeViewAvailableContent->getModel()->clear();

    // Load content from folders
    loadContentFromFolder(SystemSong::getFolder(m_songKind, false),
                          false);
    loadContentFromFolder(SystemSong::getFolder(m_songKind, true),
                          true);

    // Reselect index
    if (row != -1 && row != -2){
        QModelIndex index = ui->treeViewAvailableContent->getModel()->index(row,
                                                                            0);
        ui->treeViewAvailableContent->setCurrentIndex(index);
    }
}

// -------------------------------------------------------

void PanelSongs::loadContentFromFolder(QString path, bool isBR){
    QDir dir(path);
    QStringList files = dir.entryList(QDir::Files);
    QIcon icon = isBR ? QIcon(SuperListItem::pathIconBlue)
                      : QIcon(SuperListItem::pathIconRed);
    QStandardItem* item;
    SystemSong* super;

    for (int i = 0; i < files.size(); i++){
        super = new SystemSong(1, files.at(i), isBR);
        item = new QStandardItem;
        item->setData(
                    QVariant::fromValue(reinterpret_cast<quintptr>(super)));
        item->setIcon(icon);
        item->setText(super->name());
        ui->treeViewAvailableContent->getModel()->appendRow(item);
    }
}

// -------------------------------------------------------

void PanelSongs::deleteContent(QString path){
    if (!QFile(path).remove()){
        QMessageBox::warning(this,
                             "Warning", "Could not delete file at " + path);
    }
}

// -------------------------------------------------------

void PanelSongs::moveContent(){
    QStandardItem* selected = ui->treeViewAvailableContent->getSelected();
    SystemSong* super;

    if (selected != nullptr){
        super = (SystemSong*) selected->data().value<qintptr>();
        ui->widgetPanelIDs->list()->addNewItem(super);
    }
}

// -------------------------------------------------------

void PanelSongs::updateSongs(){
    m_song = (SystemSong*) ui->widgetPanelIDs->list()->getSelected()
            ->data().value<qintptr>();
}

// -------------------------------------------------------
//
//  SLOTS
//
// -------------------------------------------------------

void PanelSongs::showAvailableContent(bool b){
    ui->checkBoxContent->setChecked(b);
    ui->treeViewAvailableContent->setVisible(b);
    ui->pushButtonMove->setVisible(b);
    ui->pushButtonRefresh->setVisible(b);
    ui->pushButtonAdd->setVisible(b);
}

// -------------------------------------------------------

void PanelSongs::play() {
    m_mediaPlayer.play();
    ui->pushButtonStop->setEnabled(true);
    ui->pushButtonPause->setEnabled(true);
}

// -------------------------------------------------------

void PanelSongs::on_listIDsIndexChanged(QModelIndex index, QModelIndex)
{
    updateSong(ui->widgetPanelIDs->list()->getModel()->itemFromIndex(index));
}

// -------------------------------------------------------

void PanelSongs::on_listIndexChanged(QModelIndex index, QModelIndex)
{
    updateSong(ui->treeViewAvailableContent->getModel()->itemFromIndex(index));
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonMove_clicked(){
    moveContent();
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonRefresh_clicked(){
    loadAvailableContent();
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonAdd_clicked(){

    // Open dialog box
    QStringList files = QFileDialog::getOpenFileNames(
                this, "Add new contents", "", "Music (*.mp3 *.ogg, *.wav)");
    QString path;

    // Copy all the selected files
    for (int i = 0; i < files.size(); i++){
        path = files.at(i);
        if (!QFile::copy(path,
                    Common::pathCombine(
                        SystemSong::getFolder(m_songKind, false),
                        QFileInfo(path).fileName())))
        {
            QMessageBox::warning(this,
                                 "Warning", "Could not copy file at " + path);
        }
    }

    // Refresh content
    loadAvailableContent();
}

// -------------------------------------------------------

void PanelSongs::deletingContent(SuperListItem* super, int row){
    QString path = ((SystemSong*) super)->getPath(m_songKind);

    // If is BR, ask if sure action before
    if (((SystemSong*) super)->isBR()){
        loadAvailableContent(row);
        QMessageBox::StandardButton box =
                QMessageBox::question(this, "Deleting song",
                                      "You are trying to remove a BR song. "
                                      "Are you sure you want to do it?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (box == QMessageBox::Yes){
            deleteContent(path);
            loadAvailableContent();
        }
    }
    else
        deleteContent(path);
}

// -------------------------------------------------------

void PanelSongs::on_treeViewAvailableContent_doubleClicked(QModelIndex){
    play();
}

// -------------------------------------------------------

void PanelSongs::on_widgetPanelIDsDoubleClicked(QModelIndex){
    play();
}

// -------------------------------------------------------

void PanelSongs::on_checkBoxStart_toggled(bool checked) {
    ui->doubleSpinBoxStart->setEnabled(checked);
    ui->labelSeconds1->setEnabled(checked);
}

// -------------------------------------------------------

void PanelSongs::on_checkBoxEnd_toggled(bool checked) {
    ui->doubleSpinBoxEnd->setEnabled(checked);
    ui->labelSeconds2->setEnabled(checked);
}

// -------------------------------------------------------

void PanelSongs::on_spinBoxVolume_valueChanged(int value) {
    ui->horizontalSliderVolume->setValue(value);
}

// -------------------------------------------------------

void PanelSongs::on_horizontalSliderVolume_valueChanged(int value) {
    ui->spinBoxVolume->setValue(value);
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonPlay_clicked() {
    play();
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonStop_clicked() {
    m_mediaPlayer.stop();
    ui->pushButtonStop->setEnabled(false);
    ui->pushButtonPause->setEnabled(false);
}

// -------------------------------------------------------

void PanelSongs::on_pushButtonPause_clicked() {
    m_mediaPlayer.pause();
    ui->pushButtonPause->setEnabled(false);
}

// -------------------------------------------------------

void PanelSongs::on_mediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        m_mediaPlayer.stop();
        ui->pushButtonStop->setEnabled(false);
    }
}