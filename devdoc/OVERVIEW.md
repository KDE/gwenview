# "Model"

## Document

A document can hold either a raster image, an svg image or a video.

The DocumentKind enum represents what kind of document it is.

Holds the data, responsible for loading and saving.

## DocumentFactory

Creates Document instances. Can cache them.

# "View"

## DocumentView

Can display a document. Depending on the document kind, the DocumentView will
use a different adapter (see AbstractDocumentViewAdapter)

## DocumentViewContainer

DocumentViewContainer is responsible for creating and deleting DocumentViews.

Every time a new document is visible, a new DocumentView is created. When it is
no longer visible the document view is deleted. This means when user goes from
1.jpg to 2.jpg, the DocumentView displaying 1.jpg is deleted and a new one is
created for 2.jpg.

DocumentViewContainer is also responsible for laying out the different views
when comparing them.
