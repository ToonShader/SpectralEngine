//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "Run.h"

using namespace SpectralEditor;


#include <winrt/Windows.Foundation.h>
#include <ppltasks.h>
#include <experimental/filesystem>
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::Storage::Search;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;


DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_coreInput(nullptr)
{
	InitializeComponent();

	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);
	
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += 
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	//m_deviceResources = std::make_shared<DX::DeviceResources>();
	//m_deviceResources->SetSwapChainPanel(swapChainPanel);

	// Register our SwapChainPanel to get independent input pointer events
	auto workItemHandler = ref new WorkItemHandler([this] (IAsyncAction ^)
	{
		// The CoreIndependentInputSource will raise pointer events for the specified device types on whichever thread it's created on.
		m_coreInput = swapChainPanel->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
			);

		// Register for pointer events, which will be raised on the background thread.
		m_coreInput->PointerPressed += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerPressed);
		m_coreInput->PointerMoved += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerMoved);
		m_coreInput->PointerReleased += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerReleased);

		// Begin processing input messages as they're delivered.
		m_coreInput->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	});

	// Run task on a dedicated high priority background thread.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	// Start my stuff - - - -
	InitializePrimaryScene();

	m_main = std::unique_ptr<SpectralEditorMain>(new SpectralEditorMain());
	m_main->StartRenderLoop(swapChainPanel);
}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	m_main->StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	//m_main->StartRenderLoop();
}

// Window event handlers.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop(swapChainPanel);
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	//m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->ValidateDevice();
}

// Called when the app bar button is clicked.
void DirectXPage::AppBarButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	// Use the app bar if it is appropriate for your app. Design the app bar, 
	// then fill in event handlers (like this one).
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ e)
{
	// When the pointer is pressed begin tracking the pointer movement.
	//m_main->StartTracking();

	if (e->CurrentPoint->PointerDevice->PointerDeviceType == Windows::Devices::Input::PointerDeviceType::Mouse)
	{
		auto properties = e->CurrentPoint->Properties;
		if (properties->IsLeftButtonPressed)
			ActivePointerButton = 1;
		else if (properties->IsRightButtonPressed)
			ActivePointerButton = 2;
		else if (properties->IsMiddleButtonPressed)
			ActivePointerButton = 3;

		auto point = e->CurrentPoint->Position;
		OnMouseDown(ActivePointerButton, point.X, point.Y);
	}

	e->Handled = true;
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ e)
{
	// Update the pointer tracking code.
	//if (m_main->IsTracking())
	//{
	//	m_main->TrackingUpdate(e->CurrentPoint->Position.X);
	//}

	if (ActivePointerButton)
	{
		auto pos = e->CurrentPoint->Position;
		OnMouseMove(ActivePointerButton, pos.X, pos.Y);
	}

	e->Handled = true;
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ e)
{
	// Stop tracking pointer movement when the pointer is released.
	//m_main->StopTracking();

	auto pos = e->CurrentPoint->Position;
	OnMouseUp(ActivePointerButton, pos.X, pos.Y);
	ActivePointerButton = 0;

	e->Handled = true;
}

void DirectXPage::InitializePrimaryScene()
{
	// We need to tell the scene what objects to load first
	std::vector<std::string> fileNames;
	auto directoryEntries = GetFolderContents(L"Models");
	for (auto& entry : directoryEntries)
	{
		fileNames.push_back("Models/" + entry.path().filename().string());
	}
	gScene.SetObjectFiles(fileNames);
	fileNames.clear();

	// TODO: Need to separate basic geometry generation/loading from creating the vertex/index buffers
	// Now we can initialize the scene
	Initialize(swapChainPanel);

	// Update the UI panel with the objects that can be placed in the scene
	gScene.GetAvailableObjects(fileNames);
	objectPickerGrid->Children->Clear();
	for (auto& name : fileNames)
	{
		TextBlock^ tile = ref new TextBlock();
		tile->Text = ref new String(std::wstring(name.begin(), name.end()).c_str());
		tile->PointerPressed += ref new PointerEventHandler(this, &DirectXPage::ObjectPanel_PointerPressed);
		objectPickerGrid->Children->Append(tile);
	}
}



//#include "winrt/Windows.Storage.h"
//#include "winrt/Windows.Storage.Pickers.h"
//#include <Windows.h>
//#include <wrl.h>
//#include <filesystem>
//using namespace Windows::Storage;
//using namespace Windows::Storage::Pickers;
//
//#include <pplawait.h>
//
//using namespace Platform;
//
//#include <fstream>
//#include <iostream>
//#include <filesystem>

std::vector<std::experimental::filesystem::directory_entry> DirectXPage::GetFolderContents(const std::wstring& folder)
{
	Windows::Storage::StorageFolder^ installedLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;
	std::wstring path = installedLocation->Path->Data();
	path += L"/" + folder;

	std::vector<std::experimental::filesystem::directory_entry> entries;
	for (auto& entry : std::experimental::filesystem::directory_iterator(path))
		entries.push_back(entry);

	return entries;
}

//task<std::pair<std::vector<StorageFolder^>, std::vector<StorageFile^>>> GetModelFolderContents()
//{
//
//
//	// TODO: Add support for changing directories
//	//auto picker = ref new FileOpenPicker();
//	//picker->FileTypeFilter->Append(L".jpg");
//	//picker->SuggestedStartLocation = PickerLocationId::PicturesLibrary;
//
//	//auto file = co_await picker->PickSingleFileAsync();
//	//if (nullptr == file)
//	//	return;
//
//	//auto stream = co_await file->OpenReadAsync();
//
//	//OutputDebugString(L"1. End of OpenReadAsync lambda.\r\n");
//
//	//Windows::Storage::StorageFolder^ installedLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;
//	//auto folders = co_await installedLocation->GetFoldersAsync();
//	//for (auto folder : folders)
//	//{
//	//	
//	//}
//	//
//	//auto files = co_await installedLocation->GetFilesAsync();
//	//for (auto file : files)
//	//{
//
//	//}
//
//	//create_task(KnownFolders::GetFolderForUserAsync(nullptr /* current user */, KnownFolderId::PicturesLibrary)).then([this, queryOptions](StorageFolder^ picturesFolder)
//	//{
//	//	auto query = picturesFolder->CreateFileQueryWithOptions(queryOptions);
//	//	return query->GetFilesAsync();
//	//}).then([=](IVectorView<StorageFile^>^ files)
//	//{
//	//	std::for_each(begin(files), end(files), [=](StorageFile^ file)
//	//	{
//	//		// Create UI elements for the output and fill in the contents when they are available.
//	//		OutputPanel->Children->Append(CreateHeaderTextBlock(file->Name));
//	//		TextBlock^ imagePropTextBlock = CreateLineItemTextBlock();
//	//		OutputPanel->Children->Append(imagePropTextBlock);
//	//		TextBlock^ copyrightTextBlock = CreateLineItemTextBlock();
//	//		OutputPanel->Children->Append(copyrightTextBlock);
//	//		TextBlock^ colorspaceTextBlock = CreateLineItemTextBlock();
//	//		OutputPanel->Children->Append(colorspaceTextBlock);
//
//	//		// GetImagePropertiesAsync will return synchronously when prefetching has been able to
//	//		// retrieve the properties in advance.
//	//		create_task(file->Properties->GetImagePropertiesAsync()).then([=](ImageProperties^ properties)
//	//		{
//	//			imagePropTextBlock->Text = "Dimensions: " + properties->Width + "x" + properties->Height;
//	//		});
//
//	//		// Similarly, extra properties are retrieved asynchronously but may
//	//		// return immediately when prefetching has fulfilled its task.
//	//		create_task(file->Properties->RetrievePropertiesAsync(propertyNames)).then([=](IMap<String^, Object^>^ extraProperties)
//	//		{
//	//			copyrightTextBlock->Text = "Copyright: " + GetPropertyDisplayValue(extraProperties->Lookup(CopyrightProperty));
//	//			colorspaceTextBlock->Text = "Color space: " + GetPropertyDisplayValue(extraProperties->Lookup(ColorSpaceProperty));
//	//		});
//
//	//		// Thumbnails can also be retrieved and used.
//	//		/*
//	//		create_task(file->GetThumbnailAsync(thumbnailMode, requestedSize, thumbnailOptions)).then([=](StorageItemThumbnail^ thumbnail)
//	//		{
//	//			// Use the thumbnail.
//	//		});
//	//		*/
//	//	});
//	//});
//
//}


//using namespace Platform;
//
//task<std::vector<StorageFile^>> GetObjectFileList()
//{
//	Windows::Storage::StorageFolder^ installedLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;
//	auto file = co_await installedLocation->GetFolderAsync("dfgd");//.PickSingleFileAsync();
//	if (file)
//	{
//		auto tempFolder = ApplicationData::Current->TemporaryFolder;
//		auto tempFile = co_await file.CopyAsync(tempFolder, file.Name(), NameCollisionOption::GenerateUniqueName);
//		if (tempFile)
//		{
//			HRESULT hr = CreateWICTextureFromFile(..., tempFile.Path().c_str(), ...);
//			DeleteFile(tempFile.Path().c_str());
//			DX::ThrowIfFailed(hr);
//		}
//	}
//
//
//	return concurrency::create_task([]
//	{
//		StorageFolder^ appInstalledFolder = Windows::ApplicationModel::Package::Current->InstalledLocation;
//		auto a = co_await appInstalledFolder->GetFolderAsync("Assets");
//
//		Uri rssFeedUri{ L"https://blogs.windows.com/feed" };
//		SyndicationClient syndicationClient;
//		SyndicationFeed syndicationFeed = syndicationClient.RetrieveFeedAsync(rssFeedUri).get();
//		return std::wstring{ syndicationFeed.Items().GetAt(0).Title().Text() };
//	});
//
//
//
//
//	StorageFolder^ assets;
//	auto files = co_await  assets.GetFilesAsync();
//
//	StorageFolderQueryResult queryResult = appInstalledFolder.CreateFolderQuery(CommonFolderQuery::GroupByMonth);
//
//	IReadOnlyList < StorageFolderfolderList =
//		await queryResult.GetFoldersAsync();
//
//
//	foreach (StorageFolder folder in folderList)
//	{
//		IReadOnlyList < StorageFilefileList = await folder.GetFilesAsync();
//
//		// Print the month and number of files in this group.
//		outputText.AppendLine(folder.Name + " (" + fileList.Count + ")");
//
//		foreach(StorageFile file in fileList)
//		{
//			// Print the name of the file.
//			outputText.AppendLine("   " + file.Name);
//		}
//	}
//
//
//	// Reset output.
//	OutputPanel->Children->Clear();
//
//	// Set up file type filter.
//	auto fileTypeFilter = ref new Vector<String^>();
//	fileTypeFilter->Append(".jpg");
//	fileTypeFilter->Append(".png");
//	fileTypeFilter->Append(".bmp");
//	fileTypeFilter->Append(".gif");
//
//	// Create query options.
//	auto queryOptions = ref new QueryOptions(CommonFileQuery::OrderByName, fileTypeFilter);
//
//	// Set up property prefetch - use the PropertyPrefetchOptions for top-level properties
//	// and a vector for additional properties.
//	//auto propertyNames = ref new Vector<String^>();
//	//propertyNames->Append(CopyrightProperty);
//	//propertyNames->Append(ColorSpaceProperty);
//	//queryOptions->SetPropertyPrefetch(PropertyPrefetchOptions::ImageProperties, propertyNames);
//
//	// Set up thumbnail prefetch if needed, e.g. when creating a picture gallery view.
//	/*
//	const UINT requestedSize = 190;
//	const ThumbnailMode thumbnailMode = ThumbnailMode::PicturesView;
//	const ThumbnailOptions thumbnailOptions = ThumbnailOptions::UseCurrentScale;
//	queryOptions->SetThumbnailPrefetch(thumbnailMode, requestedSize, thumbnailOptions);
//	*/
//
//	// Set up the query and retrieve files.
//	create_task(KnownFolders::GetFolderForUserAsync(nullptr /* current user */, KnownFolderId::PicturesLibrary)).then([this, queryOptions](StorageFolder^ picturesFolder)
//	{
//		auto query = picturesFolder->CreateFileQueryWithOptions(queryOptions);
//		return query->GetFilesAsync();
//	}).then([=](IVectorView<StorageFile^>^ files)
//	{
//		std::for_each(begin(files), end(files), [=](StorageFile^ file)
//		{
//			// Create UI elements for the output and fill in the contents when they are available.
//			OutputPanel->Children->Append(CreateHeaderTextBlock(file->Name));
//			TextBlock^ imagePropTextBlock = CreateLineItemTextBlock();
//			OutputPanel->Children->Append(imagePropTextBlock);
//			TextBlock^ copyrightTextBlock = CreateLineItemTextBlock();
//			OutputPanel->Children->Append(copyrightTextBlock);
//			TextBlock^ colorspaceTextBlock = CreateLineItemTextBlock();
//			OutputPanel->Children->Append(colorspaceTextBlock);
//
//			// GetImagePropertiesAsync will return synchronously when prefetching has been able to
//			// retrieve the properties in advance.
//			create_task(file->Properties->GetImagePropertiesAsync()).then([=](ImageProperties^ properties)
//			{
//				imagePropTextBlock->Text = "Dimensions: " + properties->Width + "x" + properties->Height;
//			});
//
//			// Similarly, extra properties are retrieved asynchronously but may
//			// return immediately when prefetching has fulfilled its task.
//			create_task(file->Properties->RetrievePropertiesAsync(propertyNames)).then([=](IMap<String^, Object^>^ extraProperties)
//			{
//				copyrightTextBlock->Text = "Copyright: " + GetPropertyDisplayValue(extraProperties->Lookup(CopyrightProperty));
//				colorspaceTextBlock->Text = "Color space: " + GetPropertyDisplayValue(extraProperties->Lookup(ColorSpaceProperty));
//			});
//
//			// Thumbnails can also be retrieved and used.
//			/*
//			create_task(file->GetThumbnailAsync(thumbnailMode, requestedSize, thumbnailOptions)).then([=](StorageItemThumbnail^ thumbnail)
//			{
//				// Use the thumbnail.
//			});
//			*/
//		});
//	});
//}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	//m_deviceResources->SetLogicalSize(e->NewSize);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::ObjectPanel_PointerPressed(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
	TextBlock^ text = static_cast<TextBlock^>(sender);

	gUpdateLock.lock();
	std::wstring_view converter(text->Text->Data());
	gScene.AddObject(std::string(converter.begin(), converter.end()));
	gUpdateLock.unlock();
	e->Handled = true;
}
