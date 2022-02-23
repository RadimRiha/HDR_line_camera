#pragma once

namespace cameraapp {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::ComboBox^ deviceSelect;
	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->deviceSelect = (gcnew System::Windows::Forms::ComboBox());
			this->SuspendLayout();
			// 
			// deviceSelect
			// 
			this->deviceSelect->FormattingEnabled = true;
			this->deviceSelect->Location = System::Drawing::Point(12, 40);
			this->deviceSelect->Name = L"deviceSelect";
			this->deviceSelect->Size = System::Drawing::Size(365, 21);
			this->deviceSelect->TabIndex = 0;
			this->deviceSelect->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::deviceSelect_SelectedIndexChanged);
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(684, 485);
			this->Controls->Add(this->deviceSelect);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->ResumeLayout(false);

		}
#pragma endregion

	private: System::Void deviceSelect_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e) {

	}
	};
}
