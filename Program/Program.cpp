#include <iostream>
#include <fstream>
#include <filesystem>
#include <stack>
#include <functional>
#include <windows.h>

class Session;
class SessionsHistory;

class Editor {
private:
	static SessionsHistory* sessionsHistory; //історія сеансів
	static Session* currentSession; //сеанс, з яким користувач працює в даний момент
	static std::string* currentText; //текст, який користувач редагує в даний момент

public:
	Editor();

	~Editor() {
		delete sessionsHistory;
		if(currentText)
			delete (currentText);
	}

	void tryToLoadSessions();
	void tryToUnloadSessions();

	void copy(std::string textToProcess, int startPosition, int endPosition);
	void paste(std::string* textToProcess, int startPosition, int endPosition, std::string textToPaste);
	void cut(std::string* textToProcess, int startPosition, int endPosition);
	void remove(std::string* textToProcess, int startPosition, int endPosition);

	static Session* getCurrentSession();
	static std::string* getCurrentText();
	static void setCurrentSession(Session* session);
	static void setCurrentText(std::string* text);
	SessionsHistory* getSessionsHistory();

	static void printCurrentText();
};

class Command {
protected:
	Editor* editor; //редактор, в якому відбувається редагування тексту за допомогою команд
	int startPosition, endPosition; //початкова та кінцева позиції для вставки, заміни, видалення, копіювання, вирізання
	std::string textToProcess, textToPaste; //поля для тексту, який обробляємо і для тексту, який вставляємо 
	Command* previousCommand, * commandToUndoOrRedo; //вказівник на попередню команду (в історії команд щось по типу однонапрямленого списка),
	//далі - вказівник на команду, яку збираємось скасувати або повторити

public:
	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual Command* copy() = 0;

	void setParameters(std::string typeOfCommand, Command* previousCommand, Command* commandToUndoOrRedo, int startPosition, int endPosition, std::string textToPaste) {
		if (typeOfCommand == "Undo" || typeOfCommand == "Redo")
		{
			this->commandToUndoOrRedo = commandToUndoOrRedo;
			return;
		}

		if (startPosition > endPosition && endPosition > -1 && startPosition < editor->getCurrentText()->size())
			std::swap(startPosition, endPosition);

		if (startPosition == -1 && endPosition == -1) {
			startPosition = 0;
			endPosition = 0;
		}

		this->startPosition = startPosition;
		this->endPosition = endPosition;
		this->textToProcess = *(Editor::getCurrentText());
		this->previousCommand = previousCommand;

		if (typeOfCommand == "Paste")
			this->textToPaste = textToPaste;
	}

	std::string getTextToProcess() { return textToProcess; }
	void setTextToProcess(std::string textToProcess) { this->textToProcess = textToProcess; }
	void setPreviousCommand(Command* previousCommand) { this->previousCommand = previousCommand; }
};

class Session {
private:
	std::stack<Command*> commandsHistory; //історія команд
	std::stack<std::string> clipboard; //буфер обміну
	int currentCommandIndexInHistory; //індекс на команді, на якій знаходиться користувач, бо, можливо, він скасував декілька команд або повторив,
	//і це потрібно відслідковвувати
	std::string name; //ім'я сеансу

public:
	Session() { currentCommandIndexInHistory = -1; }
	Session(std::string filename) : Session() { name = filename; }
	~Session() {
		while (!commandsHistory.empty()) {
			if (commandsHistory.top())
				delete commandsHistory.top();
			commandsHistory.pop();
		}
	}

	void addCommandAsLast(Command* command) { commandsHistory.push(command); }
	void addDataToClipboard(std::string data) { clipboard.push(data); }
	void deleteLastCommand() {
		delete commandsHistory.top();
		commandsHistory.pop();
	}

	int sizeOfCommandsHistory() { return commandsHistory.size(); }
	int sizeOfClipboard() { return clipboard.size(); }

	bool setName(std::string filename) {
		std::string forbiddenCharacters = "/\\\":?*|<>";

		for (char ch : forbiddenCharacters)
			if (filename.find(ch) != std::string::npos)
				return false;

		name = filename + ".txt";
		return true;
	}
	void setCurIndexInCommHistory(int currentCommandIndexInHistory) {
		this->currentCommandIndexInHistory = currentCommandIndexInHistory;
	}

	std::string getName() { return name; }
	Command* getCommandByIndex(int index) { return commandsHistory._Get_container()[index]; }
	int getCurIndexInCommHistory() { return currentCommandIndexInHistory; }
	std::string getDataFromClipboardByIndex(int index) { return clipboard._Get_container()[index]; }

	void printClipboard() {
		system("cls");
		for (int i = 0; i < clipboard.size(); i++)
			std::cout << "\n" << i + 1 << ") \"" << clipboard._Get_container()[i] << "\"";
		std::cout << std::endl;
	}
};

class SessionsHistory {
private:
	std::stack<Session*> sessions; //історія сеансів

public:
	~SessionsHistory() {
		while (!sessions.empty()) {
			delete sessions.top();
			sessions.pop();
		}
	}

	void addSessionToEnd(Session* session) { sessions.push(session); }
	Session* getSessionByIndex(int index) { return sessions._Get_container()[index]; }
	Session* getSessionByName(std::string name) {
		auto sessionsIter = std::find_if(sessions._Get_container().begin(), sessions._Get_container().end(), [name](Session* session) {
			return session->getName() == name + ".txt" || session->getName() == name;
			});

		if (sessionsIter != sessions._Get_container().end())
			return *sessionsIter;
		else
			return nullptr;
	}
	std::string deleteSessionByIndex(int index) {
		auto ptrOnUnderlyingContainer = &sessions._Get_container();
		auto ptrOnRetiringSession = (*ptrOnUnderlyingContainer)[index];

		std::string filename = ptrOnRetiringSession->getName();

		std::deque<Session*> notRetiringElements;

		remove_copy(ptrOnUnderlyingContainer->begin(),
			ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements),
			ptrOnRetiringSession);

		sessions = std::stack<Session*>(notRetiringElements);

		delete ptrOnRetiringSession;

		return filename;
	}
	std::string deleteSessionByName(std::string name) {
		auto sessionToDelete = getSessionByName(name);
		if (!sessionToDelete)
			return "";

		Session* topSession;
		std::string filename = sessionToDelete->getName();
		std::stack<Session*> tempStack;


		while (!sessions.empty()) {
			topSession = sessions.top();
			sessions.pop();

			if (topSession->getName() == name || topSession->getName() == name + ".txt") {
				delete topSession;
				continue;
			}

			tempStack.push(topSession);
		}

		while (!tempStack.empty()) {
			sessions.push(tempStack.top());
			tempStack.pop();
		}

		return filename;
	}

	bool isEmpty() { return sessions.size() == 0; }
	int size() { return sessions.size(); }

	void printSessionsHistory() {
		system("cls");
		for (int i = 0; i < sessions.size(); i++)
			std::cout << "\nСеанс #" << i + 1 << ": " << sessions._Get_container()[i]->getName();
		std::cout << std::endl;
	}

	void sortByName() {
		std::deque<Session*> tempSessions;

		while (!sessions.empty()) {
			tempSessions.push_back(sessions.top());
			sessions.pop();
		}

		std::sort(tempSessions.begin(), tempSessions.end(), [](Session* a, Session* b) {
			return a->getName() < b->getName();
			});

		for (Session* session : tempSessions)
			sessions.push(session);
	}
};

class CopyCommand : public Command {
public:
	CopyCommand(Editor* editor);

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class DeleteCommand : public Command {
public:
	DeleteCommand(Editor* editor);

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class CutCommand : public Command {
public:
	CutCommand(Editor* editor);

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class PasteCommand : public Command {
public:
	PasteCommand(Editor* editor);

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class UndoCommand : public Command {
public:
	~UndoCommand() {
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class RedoCommand : public Command {
public:
	~RedoCommand() {
		if (commandToUndoOrRedo)
			delete (commandToUndoOrRedo);
	}

	void execute() override;
	void undo() override;
	Command* copy() override;
};

class FilesManager {
private:
	friend class Editor;

	static const std::string METADATA_DIRECTORY, //директорія папки метаданих
		DATA_DIRECTORY; //директорія, де безпосередньо збергаються текстові файли, які ми редагуємо в програмі

	static std::stack<std::string> getFilepathsForMetadata(std::string directory) {
		std::stack<std::string> filesFromMetadataDirectory;

		auto iteratorOnFiles = std::filesystem::directory_iterator(directory);

		for (const auto& entry : iteratorOnFiles)
				filesFromMetadataDirectory.push(entry.path().string());

		return filesFromMetadataDirectory;
	}
	static std::string readDataByDelimiter(std::ifstream* ifs_session, std::string delimiter) {
		std::string line, text;
		int counterOfLines = 0;

		getline(*ifs_session, line);
		while (getline(*ifs_session, line)) {
			if (line == delimiter)
				break;
			counterOfLines++;
			if (counterOfLines > 1)
				line = "\n" + line;
			text += line;
		}

		return text;
	}

	static void deleteMetadataForDeletedSessions(SessionsHistory* sessionsHistory, std::string directory) {
		if (!std::filesystem::exists(directory))
			return;

		std::stack<std::string> filesFromMetadataDirectory = getFilepathsForMetadata(directory);
		std::string sessionFilepath;
		bool isPartOfRealtimeSessions;

		for (int i = 0; i < filesFromMetadataDirectory.size(); i++)
		{
			isPartOfRealtimeSessions = false;
			for (int j = 0; j < sessionsHistory->size(); j++)
			{
				sessionFilepath = directory + sessionsHistory->getSessionByIndex(j)->getName();
				if (filesFromMetadataDirectory._Get_container()[i] == sessionFilepath)
					isPartOfRealtimeSessions = true;
			}

			if (!isPartOfRealtimeSessions)
				remove(filesFromMetadataDirectory._Get_container()[i].c_str());
		}
		
	}

	static void writeSessionsMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, METADATA_DIRECTORY);

		if (!std::filesystem::exists(METADATA_DIRECTORY))
			std::filesystem::create_directories(METADATA_DIRECTORY);

		for (int i = 0; i < sessionsHistory->size(); i++)
			writeSessionMetadata(sessionsHistory, i);
	}
	static void writeSessionMetadata(SessionsHistory* sessionsHistory, int index) {
		Session* session = sessionsHistory->getSessionByIndex(index);

		std::ofstream ofs_session(METADATA_DIRECTORY + session->getName());

		ofs_session << session->sizeOfCommandsHistory() << std::endl;
		ofs_session << session->getCurIndexInCommHistory() << std::endl;

		for (int j = 0; j < session->sizeOfCommandsHistory(); j++)
			writeCommandMetadata(&ofs_session, session, j);

		ofs_session.close();
	}
	static void writeCommandMetadata(std::ofstream* ofs_session, Session* session, int index) {
		std::string typeOfCommand, nameOfCommandClass, delimiter = "---\n";
		Command* command = session->getCommandByIndex(index);

		nameOfCommandClass = std::string(typeid(*command).name());

		if (nameOfCommandClass == "class PasteCommand")
			typeOfCommand = "PasteCommand";
		else if (nameOfCommandClass == "class CutCommand")
			typeOfCommand = "CutCommand";
		else
			typeOfCommand = "DeleteCommand";

		*ofs_session << typeOfCommand << std::endl << delimiter;

		if (command->getTextToProcess() != "")
			*ofs_session << command->getTextToProcess() << std::endl;

		*ofs_session << delimiter;
	}

	static void readSessionsMetadata(Editor* editor) {
		if (!std::filesystem::exists(METADATA_DIRECTORY))
			return;

		std::stack<std::string> available_sessions;
		std::string line;

		available_sessions = getFilepathsForMetadata(METADATA_DIRECTORY);

		for (int i = 0; i < available_sessions.size(); i++)
			readSessionMetadata(editor, available_sessions._Get_container()[i]);
	}
	static void readSessionMetadata(Editor* editor, std::string filepath) {
		std::string line;
		filepath.erase(0, METADATA_DIRECTORY.size());
		Session* session = new Session(filepath);
		int countOfCommands;

		std::ifstream ifs_session(METADATA_DIRECTORY + filepath);

		getline(ifs_session, line);
		countOfCommands = stoi(line);

		getline(ifs_session, line);
		session->setCurIndexInCommHistory(stoi(line));

		for (int j = 0; j < countOfCommands; j++)
			readCommandMetadata(editor, &ifs_session, session);

		editor->getSessionsHistory()->addSessionToEnd(session);

		ifs_session.close();
	}
	static void readCommandMetadata(Editor* editor, std::ifstream* ifs_session, Session* session) {
		std::string typeOfCommand, text;
		Command* command,* previousCommand;

		getline(*ifs_session, typeOfCommand);
		if (typeOfCommand == "CutCommand")
			command = new CutCommand(editor);
		else if (typeOfCommand == "PasteCommand")
			command = new PasteCommand(editor);
		else
			command = new DeleteCommand(editor);

		if (session->sizeOfCommandsHistory() > 0)
		{
			previousCommand = session->getCommandByIndex(session->sizeOfCommandsHistory() - 1);
			command->setPreviousCommand(previousCommand);
		}

		text = readDataByDelimiter(ifs_session, "---");
		command->setTextToProcess(text);

		session->addCommandAsLast(command);
	}

public:
	static std::string getSessionsDirectory() {
		return DATA_DIRECTORY;
	}

	static std::string readSessionData(std::string fullFilepath) {
		std::string text, line;

		std::ifstream file(fullFilepath);

		int counterOfLines = 0;

		while (getline(file, line))
		{
			counterOfLines++;
			if (counterOfLines > 1)
				line = "\n" + line;
			text += line;
		}

		file.close();

		return text;
	}

	static bool writeSessionData(std::string filename, std::string newData) {
		std::ofstream file(DATA_DIRECTORY + filename);

		if (!file.is_open())
			return false;

		file << newData;
		if(!newData.empty() && newData[newData.size() - 1] == '\n')
			file << '\n';

		file.close();

		return true;
	}
};

const std::string FilesManager::METADATA_DIRECTORY = "Metadata\\",
FilesManager::DATA_DIRECTORY = "Data\\";

void Editor::tryToLoadSessions() { FilesManager::readSessionsMetadata(this); }
void Editor::tryToUnloadSessions() { FilesManager::writeSessionsMetadata(sessionsHistory); }

Editor::Editor() { this->sessionsHistory = new SessionsHistory(); }

void Editor::copy(std::string textToProcess, int startPosition, int endPosition) {
	std::string dataToCopy = textToProcess.substr(startPosition, endPosition - startPosition + 1);
	currentSession->addDataToClipboard(dataToCopy);
}
void Editor::paste(std::string* textToProcess, int startPosition, int endPosition, std::string textToPaste) {
	if (startPosition == endPosition) {
		if(startPosition == 0)
			*textToProcess = textToPaste + *textToProcess;
		else if(startPosition == textToProcess->size() - 1)
			*textToProcess += textToPaste;
		else
			(*textToProcess).replace(startPosition, endPosition - startPosition + 1, textToPaste);
	}
	else
	{
		if(endPosition == -1)
			(*textToProcess).replace(0, 1, textToPaste);
		else if (endPosition == textToProcess->size()) 
			(*textToProcess).replace(textToProcess->size() - 1, textToProcess->size() - 1, textToPaste);
		else
			(*textToProcess).replace(startPosition, endPosition - startPosition + 1, textToPaste);
	}
	*currentText = *textToProcess;
}
void Editor::cut(std::string* textToProcess, int startPosition, int endPosition) {
	copy(*textToProcess, startPosition, endPosition);
	remove(textToProcess, startPosition, endPosition);
	*currentText = *textToProcess;
}
void Editor::remove(std::string* textToProcess, int startPosition, int endPosition) {
	(*textToProcess).erase(startPosition, endPosition - startPosition + 1);
	*currentText = *textToProcess;
}

Session* Editor::getCurrentSession() { return currentSession; }
std::string* Editor::getCurrentText() { return currentText; }
void Editor::setCurrentSession(Session* session) { currentSession = session; }
void Editor::setCurrentText(std::string* text) { currentText = text; }
SessionsHistory* Editor::getSessionsHistory() { return sessionsHistory; }

void Editor::printCurrentText() {
	system("cls");
	std::cout << "\nЗміст файлу " << currentSession->getName() << ":\n";
	*currentText != "" ? 
		std::cout << "\"" << *currentText << "\"\n" : 
		std::cout << "\nФайл пустий!\n";
}

SessionsHistory* Editor::sessionsHistory;
Session* Editor::currentSession;
std::string* Editor::currentText;

CopyCommand::CopyCommand(Editor* editor) { this->editor = editor; }

void CopyCommand::execute() { editor->copy(textToProcess, startPosition, endPosition); }
void CopyCommand::undo() { }
Command* CopyCommand::copy() { return nullptr; }

DeleteCommand::DeleteCommand(Editor* editor) { this->editor = editor; }

void DeleteCommand::execute() { editor->remove(&textToProcess, startPosition, endPosition); }
void DeleteCommand::undo() { *(Editor::getCurrentText()) = previousCommand->getTextToProcess(); }
Command* DeleteCommand::copy() { return new DeleteCommand(*this); }

CutCommand::CutCommand(Editor* editor) { this->editor = editor; }

void CutCommand::execute() { editor->cut(&textToProcess, startPosition, endPosition); }
void CutCommand::undo() { *(Editor::getCurrentText()) = previousCommand->getTextToProcess(); }
Command* CutCommand::copy() { return new CutCommand(*this); }

PasteCommand::PasteCommand(Editor* editor) { this->editor = editor; }

void PasteCommand::execute() { editor->paste(&textToProcess, startPosition, endPosition, textToPaste); }
void PasteCommand::undo() {
	if (previousCommand)
		*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
	else
		*(Editor::getCurrentText()) = "";
}
Command* PasteCommand::copy() { return new PasteCommand(*this); }

void UndoCommand::execute() { commandToUndoOrRedo->undo(); }
void UndoCommand::undo() { }
Command* UndoCommand::copy() { return nullptr; }

void RedoCommand::execute() { *(Editor::getCurrentText()) = commandToUndoOrRedo->getTextToProcess(); }
void RedoCommand::undo() { }
Command* RedoCommand::copy() { return nullptr; }

class CommandsManager {
private:
	std::stack<std::pair<std::string, Command*>> manager; //зберігач усіх команд, дозволяє зручно їми керувати за допомогою поліморфізму

	Command* getCommandFromManagerByKey(std::string typeOfCommand) {
		for (int i = 0; i < manager.size(); i++)
			if (manager._Get_container()[i].first == typeOfCommand)
				return manager._Get_container()[i].second;
	}
	bool isNotUndoOrRedoCommand(std::string typeOfCommand) { return typeOfCommand != "Undo" && typeOfCommand != "Redo"; }
	void setParametersForCommand(std::string typeOfCommand, int startPosition, int endPosition, std::string textToPaste) {
		Command* commandToUndoOrRedo = nullptr, * previousCommand = nullptr;

		if (typeOfCommand == "Undo")
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurIndexInCommHistory());

		if(typeOfCommand == "Redo")
			commandToUndoOrRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurIndexInCommHistory() + 1);

		if (Editor::getCurrentSession()->getCurIndexInCommHistory() != -1 && isNotUndoOrRedoCommand(typeOfCommand))
			previousCommand = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->sizeOfCommandsHistory() - 1);

		

		getCommandFromManagerByKey(typeOfCommand)->setParameters(typeOfCommand, previousCommand, commandToUndoOrRedo, startPosition, endPosition, textToPaste);
	}
	int getCountOfForwardCommands() {
		return Editor::getCurrentSession()->sizeOfCommandsHistory() - 1 - Editor::getCurrentSession()->getCurIndexInCommHistory();
	}
	void deleteForwardCommandsIfNecessary(std::string typeOfCommand) {
		if (typeOfCommand != "Undo" && typeOfCommand != "Redo" && isThereAnyCommandForward())
			for (int i = 0; i <= getCountOfForwardCommands(); i++)
				Editor::getCurrentSession()->deleteLastCommand();
	}

public:
	CommandsManager(Editor* editor) {
		manager.push(std::pair("Copy", new CopyCommand(editor)));
		manager.push(std::pair("Paste", new PasteCommand(editor)));
		manager.push(std::pair("Cut", new CutCommand(editor)));
		manager.push(std::pair("Delete", new DeleteCommand(editor)));
		manager.push(std::pair("Undo", new UndoCommand()));
		manager.push(std::pair("Redo", new RedoCommand()));
	}
	~CommandsManager() {
		while (!manager.empty())
		{
			delete manager.top().second;
			manager.pop();
		}
	}

	bool isThereAnyCommandForward() {
		return Editor::getCurrentSession()->sizeOfCommandsHistory() != 0 &&
			Editor::getCurrentSession()->getCurIndexInCommHistory() < Editor::getCurrentSession()->sizeOfCommandsHistory() - 1;
	}
	void invokeCommand(std::string typeOfCommand, int startPosition = 0, int endPosition = 0, std::string textToPaste = "") {

		deleteForwardCommandsIfNecessary(typeOfCommand);
		setParametersForCommand(typeOfCommand, startPosition, endPosition, textToPaste);

		getCommandFromManagerByKey(typeOfCommand)->execute();

		if (isNotUndoOrRedoCommand(typeOfCommand) && typeOfCommand != "Copy")
			Editor::getCurrentSession()->addCommandAsLast(getCommandFromManagerByKey(typeOfCommand)->copy());

		if(typeOfCommand == "Undo")
			Editor::getCurrentSession()->setCurIndexInCommHistory(Editor::getCurrentSession()->getCurIndexInCommHistory() - 1);
		else
			if (typeOfCommand != "Copy")
				Editor::getCurrentSession()->setCurIndexInCommHistory(Editor::getCurrentSession()->getCurIndexInCommHistory() + 1);
	}
};

Editor* editor; //редактор
CommandsManager* commandsManager; //менеджер команд, за допомогою якого й викликаються усі команди

bool validateEnteredNumber(std::string option, short firstOption, short lastOption);
int enterNumberInRange(std::string message, int firstOption, int lastOption);
void readDataFromFile();
void pauseAndCleanConsole();
void printNotification(std::string type, std::string msg);
void successNotification(std::string msg);
void errorNotification(std::string msg);

void templateForMenusAboutSessions(int& choice, std::string action);
void templateForExecutingMenusAboutSessions(std::function<void(int&)> mainFunc, std::function<void(int&)> menu,
	std::function<bool()> actionFuncByName, std::function<void()> additionalFunc);

std::string getTextUsingKeyboard();
std::string getTextFromClipboard();

bool makeActionOnContextByEnteredText(std::string typeOfCommand, std::string actionInPast,
	std::string textToPaste, size_t startIndex, size_t endIndex);

bool undoAction();
bool redoAction();

void sortSessions();

void wayToGetTextForAddingMenu(int& choice);
void getTextFromClipboardMenu(int& choice);
void wayToPasteTextMenu(int& choice);
void delCopyOrCutTextMenu(int& choice, std::string action);
void makeActionsOnContentMenu(int& choice);
void printGettingSessionsMenu(int& choice);
void printDeletingSessionsMenu(int& choice);
void printManagingSessionsMenu(int& choice);

std::string executeGettingTextForAdding();
bool executeAddingTextToFile();
void executeMakeActionsOnContentMenu();
void executeDeletingSessionsMenu();
void executeGettingSessionsMenu();
bool executeDelCopyOrCutText(std::string typeOfCommand, std::string actionForMenu, std::string actionInPast);
bool chooseRootDelCopyOrCut(std::string action);

bool tryToEnterIndexForSession(int& index);
bool doesAnySessionExist();

void createSession();
void setCurrentSessionByIndex(int& index);
bool setCurrentSessionByName();
void deleteSessionByIndex(int index);
bool deleteSessionByName();

void executeMainMenu();


bool validateEnteredNumber(std::string option, short firstOption, short lastOption) {
	if (option.empty())
		return false;

	for (char num : option)
		if (num < '0' || num > '9')
			return false;

	auto convertedValue = std::stoull(option);

	return firstOption <= convertedValue && convertedValue <= lastOption;
}
int enterNumberInRange(std::string message, int firstOption, int lastOption) {
	bool isOptionVerified = false;
	std::string option;

	std::cout << "\n" << message;
	getline(std::cin, option);

	isOptionVerified = validateEnteredNumber(option, firstOption, lastOption);

	if (!isOptionVerified)
		printNotification("error", "були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!");

	return isOptionVerified ? stoi(option) : -1;
}
void readDataFromFile() {
	std::string filepath = FilesManager::getSessionsDirectory() + editor->getCurrentSession()->getName();
	std::string textFromFile = FilesManager::readSessionData(filepath);
	editor->setCurrentText(new std::string(textFromFile));
}
void pauseAndCleanConsole() {
	system("pause");
	system("cls");
}
void printNotification(std::string type, std::string msg) {
	if (type == "success")
		successNotification(msg);
	else
		errorNotification(msg);
	pauseAndCleanConsole();
}
void successNotification(std::string msg) {
	std::cout << "\nУспіх: " << msg << "\n\n";
}
void errorNotification(std::string msg) {
	std::cout << "\nПомилка: " << msg << "\n\n";
}

void templateForMenusAboutSessions(int& choice, std::string action) {
	std::cout << "\nЯкий сеанс хочете " << action << ":\n";
	std::cout << "0. Назад\n";
	std::cout << "1. Останній\n";
	std::cout << "2. Найперший\n";
	std::cout << "3. За позицією\n";
	std::cout << "4. За іменем\n";
	std::cout << "5. Відсортувати сеанси за іменем\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 5);
}
void templateForExecutingMenusAboutSessions(std::function<void(int&)> mainFunc, std::function<void(int&)> menu, 
	std::function<bool()> actionFuncByName, std::function<void()> additionalFunc = nullptr) {
	int choice, index = -1;
	bool isActionSuccessfull = false;

	do
	{
		editor->getSessionsHistory()->printSessionsHistory();
		menu(choice);
		switch (choice)
		{
		case -1: continue;
		case 0:
			std::cout << "\nПовернення до Головного меню.\n\n";
			system("pause");
			return;
		default:
			if (doesAnySessionExist())
			{
				switch (choice)
				{
				case 1:
				case 2:
				case 3:
					index = choice == 1 ? editor->getSessionsHistory()->size() : choice == 2 ? 1 : -1;
					mainFunc(index);
					if (index != -1 && additionalFunc)
						additionalFunc();
					continue;
				case 4:
					isActionSuccessfull = actionFuncByName();
					if (isActionSuccessfull && additionalFunc)
						additionalFunc();
					continue;
				case 5:
					sortSessions();
				}
			}
		}
	} while (true);
}

std::string getTextUsingKeyboard() {
	std::string line, text;
	int countOfLines = 0;

	std::cout << "\nВведіть текст (зупинити - з наступного рядка введіть \"-1\"): \n";
	while (getline(std::cin, line)) {
		if (line == "-1")
			break;
		else {
			countOfLines++;
			if (countOfLines > 1)
				text += "\n" + line;
			else
				if (line == "")
					text += "\n";
				else
					text += line;
		}
	}

	return text;
}
std::string getTextFromClipboard() {
	if (editor->getCurrentSession()->sizeOfClipboard() == 0) {
		printNotification("error", "в буфері обміну ще немає даних!");
		return "";
	}

	int choice, sizeOfClipboard = editor->getCurrentSession()->sizeOfClipboard();

	editor->getCurrentSession()->printClipboard();
	getTextFromClipboardMenu(choice);

	switch (choice)
	{
	case 0:
		std::cout << "\nПовернення до меню вибору способа додавання текста.\n\n";
		system("pause");
		return "";
	case 1:
		return editor->getCurrentSession()->getDataFromClipboardByIndex(sizeOfClipboard - 1);
	case 2:
		return editor->getCurrentSession()->getDataFromClipboardByIndex(0);
	case 3:
		choice = enterNumberInRange("Введіть номер даних: ", 1, sizeOfClipboard);
		if (choice != -1)
			return editor->getCurrentSession()->getDataFromClipboardByIndex(choice - 1);
		return "";
	default:
		return "";
	}
}

bool makeActionOnContextByEnteredText(std::string typeOfCommand, std::string actionInPast, 
	std::string textToPaste = "", size_t startIndex = -2, size_t endIndex = -2) {
	if (startIndex == -2 && endIndex == -2) {
		if (editor->getCurrentText()->empty()) {
			printNotification("error", "немає тексту, який можна було б замінити!");
			return false;
		}

		std::string textForAction;

		textForAction = getTextUsingKeyboard();

		if (textForAction.empty()) {
			printNotification("error", "текст не був введений!");
			return false;
		}

		startIndex = editor->getCurrentText()->find(textForAction);

		if (startIndex == std::string::npos) {
			printNotification("error", "текст не був знайдений!");
			return false;
		}

		if (typeOfCommand == "Paste") {
			if (startIndex == 0 && textForAction.size() == 1)
				endIndex = -1;
			else if (startIndex == editor->getCurrentText()->size() - 1 && textForAction.size() == 1)
				endIndex = editor->getCurrentText()->size();
			else
				endIndex = startIndex + textForAction.size() - 1;
		}
		else {
			if (textForAction.size() == 1)
				endIndex = startIndex;
			else
				endIndex = startIndex + textForAction.size() - 1;
		}
	}

	commandsManager->invokeCommand(typeOfCommand, startIndex, endIndex, textToPaste);
	printNotification("success", "дані були успішно " + actionInPast + "!");
	return true;
}

bool undoAction() {
	if (editor->getCurrentSession()->sizeOfCommandsHistory() > 0 && editor->getCurrentSession()->getCurIndexInCommHistory() != -1)
	{
		commandsManager->invokeCommand("Undo");
		printNotification("success", "команда була успішно скасована!");
		return true;
	}
	printNotification("error", "немає дій, які можна було б скасувати!");
	return false;
}
bool redoAction() {
	bool isThereAnyCommandForward = commandsManager->isThereAnyCommandForward();
	if (isThereAnyCommandForward)
	{
		commandsManager->invokeCommand("Redo");
		printNotification("success", "команда була успішно повторена!");

	}
	else
		printNotification("error", "немає дій, які можна було б повторити!");
	return isThereAnyCommandForward;
}
void sortSessions() {
	editor->getSessionsHistory()->sortByName();
	printNotification("success", "сеанси були успішно відсортовані!");
}

void wayToGetTextForAddingMenu(int& choice) {
	std::cout << "\nЯк ви хочете додати текст:\n";
	std::cout << "0. Назад\n";
	std::cout << "1. Ввівши з клавіатури\n";
	std::cout << "2. З буферу обміну\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 2);
}
void getTextFromClipboardMenu(int& choice) {
	std::cout << "\nЯкі дані бажаєте отримати з буферу обміну:\n";
	std::cout << "0. Назад\n";
	std::cout << "1. Останні\n";
	std::cout << "2. Найперші\n";
	std::cout << "3. За індексом\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 3);
}
void wayToPasteTextMenu(int& choice) {
	std::cout << "\nЯк ви хочете вставити текст:\n";
	std::cout << "0. Вийти в Меню дій над змістом\n";
	std::cout << "1. В кінець\n";
	std::cout << "2. На початок\n";
	std::cout << "3. Ввести з клавіатури текст, який хочете замінити\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 3);
}
void delCopyOrCutTextMenu(int& choice, std::string action) {
	std::cout << "\nСкільки хочете " << action << ":\n";
	std::cout << "0. Назад\n";
	std::cout << "1. Весь зміст\n";
	std::cout << "2. Введу з клавіатури, що " << action << "\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 2);
}
void makeActionsOnContentMenu(int& choice) {
	std::cout << "\nМеню дій над змістом:\n";
	std::cout << "0. Назад\n";
	std::cout << "1. Додати текст\n";
	std::cout << "2. Видалити текст\n";
	std::cout << "3. Копіювати текст\n";
	std::cout << "4. Вирізати текст\n";
	std::cout << "5. Скасувати команду\n";
	std::cout << "6. Повторити команду\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 6);
}
void printGettingSessionsMenu(int& choice) {
	templateForMenusAboutSessions(choice, "отримати");
}
void printDeletingSessionsMenu(int& choice) {
	templateForMenusAboutSessions(choice, "видалити");
}
void printManagingSessionsMenu(int& choice) {
	system("cls");
	std::cout << "Головне меню:\n";
	std::cout << "0. Закрити програму\n";
	std::cout << "1. Створити сеанс\n";
	std::cout << "2. Відкрити сеанс\n";
	std::cout << "3. Видалити сеанс\n";
	choice = enterNumberInRange("Ваш вибір: ", 0, 3);
}

std::string executeGettingTextForAdding() {
	std::string textToPaste;
	int choice;
	do
	{
		editor->printCurrentText();
		wayToGetTextForAddingMenu(choice);

		switch (choice)
		{
		case 0:
			std::cout << "\nПовернення до Меню дій над змістом.\n\n";
			system("pause");
			return "";
		case 1:
		case 2:
			textToPaste = choice == 1 ? getTextUsingKeyboard() : getTextFromClipboard();

			if (textToPaste.empty())
				continue;
			else
				return textToPaste;
		}
	} while (true);
}
bool executeAddingTextToFile() {
	std::string textToPaste = executeGettingTextForAdding();
	if (textToPaste == "") return false;

	int choice;

	do {
		editor->printCurrentText();
		wayToPasteTextMenu(choice);
		switch (choice) {
		case 0:
			std::cout << "\nПовернення до Меню дій над змістом.\n\n";
			system("pause");
			return false;
		case 1:
			makeActionOnContextByEnteredText("Paste", "вставлені", textToPaste, editor->getCurrentText()->size() - 1, editor->getCurrentText()->size() - 1);
			return true;
		case 2:
			makeActionOnContextByEnteredText("Paste", "вставлені", textToPaste, 0, 0);
			return true;
		case 3:
			if (makeActionOnContextByEnteredText("Paste", "вставлені", textToPaste))
				return true;
		}
	} while (true);
}
void executeMakeActionsOnContentMenu() {
	commandsManager = new CommandsManager(editor);
	bool wasTextSuccessfullyChanged;
	int choice;

	readDataFromFile();

	do
	{
		wasTextSuccessfullyChanged = false;
		editor->printCurrentText();
		makeActionsOnContentMenu(choice);

		switch (choice)
		{
		case 0:
			std::cout << "\nПовернення до Меню для отримання сеансу.\n\n";
			system("pause");
			delete (commandsManager);
			return;
		case 1:
			wasTextSuccessfullyChanged = executeAddingTextToFile();
			break;
		case 2:
			wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("видалити");
			break;
		case 3:
			wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("скопіювати");
			break;
		case 4:
			wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("вирізати");
			break;
		case 5:
			wasTextSuccessfullyChanged = undoAction();
			break;
		case 6:
			wasTextSuccessfullyChanged = redoAction();
		}
		if (wasTextSuccessfullyChanged)
			FilesManager::writeSessionData(editor->getCurrentSession()->getName(), *(editor->getCurrentText()));
	} while (true);
}
void executeDeletingSessionsMenu() {
	std::function<void(int)> mainFunc = [](int index) {
		deleteSessionByIndex(index);
		};
	std::function<void(int&)> menu = [](int& choice) {
		printDeletingSessionsMenu(choice);
		};
	std::function<bool()> actionFuncByName = []() {
		return deleteSessionByName();
		};

	templateForExecutingMenusAboutSessions(mainFunc, menu, actionFuncByName);
}
void executeGettingSessionsMenu() {
	std::function<void(int&)> mainFunc = [](int& index) {
		setCurrentSessionByIndex(index);
		};
	std::function<void(int&)> menu = [](int& choice) {
		printGettingSessionsMenu(choice);
		};
	std::function<bool()> actionFuncByName = []() {
		return setCurrentSessionByName();
		};
	std::function<void()> additionalFunc = []() {
		executeMakeActionsOnContentMenu();
		};

	templateForExecutingMenusAboutSessions(mainFunc, menu, actionFuncByName, additionalFunc);
}
bool executeDelCopyOrCutText(std::string typeOfCommand, std::string actionForMenu, std::string actionInPast) {
	int choice;
	bool wasOperationSuccessful = false;

	editor->printCurrentText();
	delCopyOrCutTextMenu(choice, actionForMenu);

	switch (choice)
	{
	case 0:
		std::cout << "\nПовернення до Меню дій над змістом.\n\n";
		system("pause");
		return false;
	case 1:
		wasOperationSuccessful = makeActionOnContextByEnteredText(typeOfCommand, actionInPast, "", 0, editor->getCurrentText()->size() - 1);
		return wasOperationSuccessful;
	case 2:
		wasOperationSuccessful = makeActionOnContextByEnteredText(typeOfCommand, actionInPast);
		return wasOperationSuccessful;
	}
}
bool chooseRootDelCopyOrCut(std::string action) {
	if (editor->getCurrentText()->size() == 0)
	{
		printNotification("error", "немає тексту, який можна було б " + action + "!");
		return false;
	}
	else
	{
		if (action == "видалити")
			return executeDelCopyOrCutText("Delete", action, "видалені");
		else if (action == "скопіювати")
			return executeDelCopyOrCutText("Copy", action, "скопійовані");
		else
			return executeDelCopyOrCutText("Cut", action, "вирізані");
	}
}

bool tryToEnterIndexForSession(int& index) {
	if (index == -1) {
		index = enterNumberInRange("Введіть номер сеансу: ", 1, editor->getSessionsHistory()->size());
		if (index == -1)
			return false;
	}
	return true;
}
bool doesAnySessionExist() {
	if (editor->getSessionsHistory()->isEmpty())
		printNotification("error", "в даний момент жодного сеансу немає!");
	return !editor->getSessionsHistory()->isEmpty();
}

void createSession() {
	std::string filename;

	std::cout << "\nВведіть ім'я сеансу (заборонені символи: /\\\":?*|<>): ";
	getline(std::cin, filename);

	Session* newSession = new Session();
	if (newSession->setName(filename))
	{
		if (editor->getSessionsHistory()->getSessionByName(filename) != nullptr) {
			delete newSession;
			printNotification("error", "сеанс з таким іменем вже існує!");
			return;
		}

		if (!std::filesystem::exists(FilesManager::getSessionsDirectory()))
			std::filesystem::create_directories(FilesManager::getSessionsDirectory());

		std::string filepath = FilesManager::getSessionsDirectory() + newSession->getName();
		std::ofstream file(filepath);
		file.close();
		editor->getSessionsHistory()->addSessionToEnd(newSession);
		printNotification("success", "сеанс був успішно створений!");
	}
	else
	{
		delete newSession;
		printNotification("error", "були введені заборонені символи!");
	}
}
void setCurrentSessionByIndex(int& index) {
	if (tryToEnterIndexForSession(index))
		editor->setCurrentSession(editor->getSessionsHistory()->getSessionByIndex(index - 1));
}
bool setCurrentSessionByName() {
	std::string name;
	std::cout << "\nВведіть ім'я сеансу: ";
	getline(std::cin, name);
	auto session = editor->getSessionsHistory()->getSessionByName(name);
	if (session == nullptr)
		printNotification("error", "сеанса з таким іменем не існує!");
	else
		editor->setCurrentSession(session);
	return session != nullptr;
}
void deleteSessionByIndex(int index = -1) {
	if (tryToEnterIndexForSession(index)) {
		std::string nameOfSession = editor->getSessionsHistory()->deleteSessionByIndex(index - 1);
		std::string pathToSession = FilesManager::getSessionsDirectory() + nameOfSession;
		remove(pathToSession.c_str());

		printNotification("success", "сеанс був успішно видалений!");
	}
}
bool deleteSessionByName() {
	std::string name;
	std::cout << "\nВведіть ім'я сеансу: ";
	getline(std::cin, name);
	auto filename = editor->getSessionsHistory()->deleteSessionByName(name);
	if (filename.empty())
	{
		printNotification("error", "сеанса з таким іменем не існує!");
		return false;
	}
	std::string pathToSession = FilesManager::getSessionsDirectory() + filename;
	remove(pathToSession.c_str());

	printNotification("success", "сеанс був успішно видалений!");
	return true;
}

void executeMainMenu() {
	int choice;
	editor = new Editor();
	editor->tryToLoadSessions();

	do
	{
		printManagingSessionsMenu(choice);
		switch (choice)
		{
		case 0:
			std::cout << "\nДо побачення!\n";
			editor->tryToUnloadSessions();
			delete editor;
			return;
		case 1:
			createSession();
			continue;
		case 2:
		case 3:
			if (doesAnySessionExist())
				choice == 2 ? executeGettingSessionsMenu() :
				executeDeletingSessionsMenu();
		}

	} while (true);
}

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	executeMainMenu();
}