lessThan(QT_MAJOR_VERSION, 5) {
	error("Tanglet requires Qt 5.9 or greater")
}
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
	error("Tanglet requires Qt 5.9 or greater")
}

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = tools wordlists src
